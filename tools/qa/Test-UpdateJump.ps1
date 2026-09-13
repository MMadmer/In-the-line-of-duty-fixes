[CmdletBinding()]
param(
    [string]$Updater = (Join-Path $PSScriptRoot '..\..\build\updater\InTheLineOfDutyFixesUpdater.exe'),
    [string]$RestartStub = (Join-Path $PSScriptRoot '..\..\build\Release\ild_restart_stub.exe'),
    [int]$Port = 8793
)

# An installed client takes the newest release on the list, however many it has missed. This drives the helper the
# game starts - release list, choice of archive, download and the hand-over to the applier - across releases the
# installation never had: past an intermediate release, past a withdrawn one whose successor's patch cannot apply,
# with a patch that does apply, and with the updater inside the target archive doing the applying.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not (Test-Path -LiteralPath $Updater) -or -not (Test-Path -LiteralPath $RestartStub)) {
    throw 'Build the updater and ild_restart_stub targets first.'
}
$updater = (Resolve-Path -LiteralPath $Updater).Path
$stub = (Resolve-Path -LiteralPath $RestartStub).Path
$mock = Join-Path $PSScriptRoot 'Start-UpdateApiMock.ps1'
$shell = (Get-Process -Id $PID).Path
$root = Join-Path $repo ('qa\update-jump-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$utf8 = [Text.UTF8Encoding]::new($false)
$prefix = 'In-the-line-of-duty-fixes-'
$script:checks = 0

# The helper judges what is newer by the version compiled into it, so the releases here count up from that one.
$installed = [version](Get-Item -LiteralPath $updater).VersionInfo.ProductVersion
$base = '{0}.{1}.{2}' -f $installed.Major, $installed.Minor, $installed.Build
$next = '{0}.{1}.{2}' -f $installed.Major, $installed.Minor, ($installed.Build + 1)
$last = '{0}.{1}.{2}' -f $installed.Major, $installed.Minor, ($installed.Build + 2)

function Assert-That([bool]$Condition, [string]$Message) {
    ++$script:checks
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}

function Write-Text([string]$Path, [string]$Text) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    [IO.File]::WriteAllText($Path, $Text, $utf8)
}

function Read-Text([string]$Path) {
    Get-Content -LiteralPath $Path -Raw
}

function Test-File([string]$Game, [string]$Relative) {
    Test-Path -LiteralPath (Join-Path $Game $Relative)
}

function Get-InstalledVersion([string]$Game) {
    (Read-Text (Join-Path $Game '.ild-fixes\version.txt')).Trim()
}

function Wait-Until([scriptblock]$Condition, [string]$What, [int]$Seconds = 30) {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    while (-not (& $Condition)) {
        if ([DateTime]::UtcNow -gt $deadline) { throw "FAIL: timed out waiting for $What" }
        Start-Sleep -Milliseconds 100
    }
}

function Get-Relative([string]$Tree, [IO.FileInfo]$File) {
    $File.FullName.Substring($Tree.Length + 1).Replace('\', '/')
}

# A release in the packaged layout: payload, version, ownership list and full manifest.
function New-Tree([string]$Scenario, [string]$Version, [string[]]$Extra, [string]$CarriedUpdater) {
    $tree = Join-Path $root "$Scenario\tree-$Version"
    New-Item -ItemType Directory -Force -Path $tree | Out-Null
    Copy-Item -LiteralPath $CarriedUpdater -Destination (Join-Path $tree 'InTheLineOfDutyFixesUpdater.exe')
    Write-Text (Join-Path $tree 'bin\dinput8.dll') "loader-$Version"
    Write-Text (Join-Path $tree 'gamedata\scripts\ild_fix_ui.script') 'unchanged'
    foreach ($relative in $Extra) { Write-Text (Join-Path $tree $relative) "$relative $Version" }
    Write-Text (Join-Path $tree '.ild-fixes\version.txt') "$Version`n"
    $files = @(@(Get-ChildItem -LiteralPath $tree -File -Recurse | ForEach-Object { Get-Relative $tree $_ }) +
        @('.ild-fixes/managed-files.txt', '.ild-fixes/update-manifest.txt') | Sort-Object)
    Write-Text (Join-Path $tree '.ild-fixes\managed-files.txt') (($files -join "`n") + "`n")
    $rows = foreach ($relative in $files) {
        if ($relative -eq '.ild-fixes/update-manifest.txt') { continue }
        $file = Get-Item -LiteralPath (Join-Path $tree $relative)
        "{0}`t{1}`t{2}" -f (Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant(), $file.Length, $relative
    }
    $header = @('schema=ild-fixes.update/1', "version=$Version")
    Write-Text (Join-Path $tree '.ild-fixes\update-manifest.txt') (($header + $rows) -join "`n")
    return $tree
}

function New-Full([string]$Scenario, [string]$Tree, [string]$Version) {
    $releases = Join-Path $root "$Scenario\releases"
    New-Item -ItemType Directory -Force -Path $releases | Out-Null
    $archive = Join-Path $releases "$prefix$Version-Setup_Manual.zip"
    [IO.Compression.ZipFile]::CreateFromDirectory($Tree, $archive)
    return $archive
}

# Only what changed since the base, with a manifest that still describes the whole release, as Build-Patch cuts it.
function New-Patch([string]$Scenario, [string]$Tree, [string]$BaseTree, [string]$Version, [string]$BaseVersion) {
    $archive = Join-Path $root "$Scenario\releases\$prefix$Version-Update_Patch.zip"
    $rows = @(Get-Content -LiteralPath (Join-Path $Tree '.ild-fixes\update-manifest.txt') | Select-Object -Skip 2)
    $zip = [IO.Compression.ZipFile]::Open($archive, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in Get-ChildItem -LiteralPath $Tree -File -Recurse) {
            $relative = Get-Relative $Tree $file
            $before = Join-Path $BaseTree $relative
            $same = (Test-Path -LiteralPath $before) -and
                (Get-FileHash -LiteralPath $before).Hash -eq (Get-FileHash -LiteralPath $file.FullName).Hash
            if ($same -or $relative -eq '.ild-fixes/update-manifest.txt') { continue }
            [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $relative)
        }
        $header = @('schema=ild-fixes.update/2', "version=$Version", 'kind=patch', "base=$BaseVersion")
        $writer = [IO.StreamWriter]::new($zip.CreateEntry('.ild-fixes/update-manifest.txt').Open(), $utf8)
        try { $writer.Write(($header + $rows) -join "`n") } finally { $writer.Dispose() }
    }
    finally { $zip.Dispose() }
    return $archive
}

function New-Install([string]$Scenario, [string]$Tree) {
    $game = Join-Path $root "$Scenario\game"
    Copy-Item -LiteralPath $Tree -Destination $game -Recurse
    Copy-Item -LiteralPath $stub -Destination (Join-Path $game 'bin\XR_3DA.exe')
    Write-Text (Join-Path $game 'gamedata\scripts\_g.script') 'original mod'
    Write-Text (Join-Path $game 'appdata\savedgames\all.sav') 'original save'
    return $game
}

function Test-Entry([string]$Archive, [string]$Name) {
    $zip = [IO.Compression.ZipFile]::OpenRead($Archive)
    try { return [bool]$zip.GetEntry($Name) } finally { $zip.Dispose() }
}

function Start-Mock([string]$Scenario) {
    $script:Port += 1
    $releases = Join-Path $root "$Scenario\releases"
    $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$mock`" -Port $script:Port -ReleaseDirectory `"$releases`""
    $process = Start-Process -FilePath $shell -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    while ($true) {
        try {
            $null = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:$script:Port/releases" -TimeoutSec 2
            return $process
        }
        catch {
            if ($process.HasExited -or [DateTime]::UtcNow -gt $deadline) { throw 'The mock release API did not start.' }
            Start-Sleep -Milliseconds 200
        }
    }
}

function Start-Helper([string]$Game, [int]$Session, [string]$Mode) {
    $arguments = "$Mode --game-dir `"$Game`" --game-pid $Session --restart-exe `"$Game\bin\XR_3DA.exe`" " +
        "--restart-args `"`" --qa --api http://127.0.0.1:$script:Port/releases --lang en"
    $helper = Join-Path $Game 'InTheLineOfDutyFixesUpdater.exe'
    $process = Start-Process -FilePath $helper -ArgumentList $arguments -WindowStyle Hidden -PassThru
    # The exit code is only kept once the handle has been taken.
    $null = $process.Handle
    return $process
}

function Read-Status([string]$Game, [int]$Session) {
    $fields = @{}
    try { $lines = [IO.File]::ReadAllLines((Join-Path $Game ".ild-fixes\runtime\status-$Session.txt")) }
    catch { return $fields }
    foreach ($line in $lines) {
        $separator = $line.IndexOf('=')
        if ($separator -gt 0) { $fields[$line.Substring(0, $separator)] = $line.Substring($separator + 1) }
    }
    return $fields
}

function Send-Command([string]$Game, [int]$Session, [string]$Action) {
    $path = Join-Path $Game ".ild-fixes\runtime\command-$Session.txt"
    # The helper drops a command it reads half-written, so it only ever sees the complete file.
    Write-Text "$path.tmp" "session=$Session`naction=$Action`n"
    Move-Item -LiteralPath "$path.tmp" -Destination $path -Force
}

# Offer, download and install, the way the update window asks for them; the game then closes, as it does on install.
function Invoke-Update([string]$Scenario, [string]$Game) {
    $api = Start-Mock $Scenario
    # A process that stays up until it is stopped stands in for the game the helper watches.
    $ping = Join-Path $env:SystemRoot 'System32\PING.EXE'
    $running = Start-Process -FilePath $ping -ArgumentList '-n 600 127.0.0.1' -WindowStyle Hidden -PassThru
    try {
        $session = $running.Id
        $helper = Start-Helper $Game $session '--service'
        Wait-Until { $helper.HasExited -or (Read-Status $Game $session).state -eq 'available' } "the $Scenario offer"
        $status = Read-Status $Game $session
        if ($status.state -ne 'available') {
            throw "FAIL: the $Scenario helper offers nothing, state=$($status.state) $($status.error)"
        }
        Send-Command $Game $session 'download'
        Wait-Until { (Read-Status $Game $session).state -in 'ready', 'download_failed' } "the $Scenario download"
        $status = Read-Status $Game $session
        if ($status.state -eq 'ready') {
            Send-Command $Game $session 'apply'
            Wait-Until { $helper.HasExited } "the $Scenario hand-over"
        }
        return $status
    }
    finally {
        Stop-Process -Id $running.Id -Force -ErrorAction SilentlyContinue
        Stop-Process -Id $api.Id -Force -ErrorAction SilentlyContinue
    }
}

function Wait-Restart([string]$Game) {
    $proof = Join-Path $Game 'bin\restart-proof.txt'
    $result = Join-Path $Game '.ild-fixes\runtime\apply-result.txt'
    Wait-Until { (Test-Path -LiteralPath $proof) -or (Test-Path -LiteralPath $result) } 'the update to finish' 60
    $reason = if (Test-Path -LiteralPath $result) { (Read-Text $result).Trim() -replace "`r?`n", ' | ' }
    Assert-That (Test-Path -LiteralPath $proof) "the game restarts on the updated installation $reason"
}

try {
    # Past an intermediate release: the newest is taken whole, with every file added or dropped on the way.
    $a = New-Tree 'skip' $base @('bin/ild_fixes_dropped.txt') $updater
    $b = New-Tree 'skip' $next @('bin/ild_fixes_intermediate.txt') $updater
    $c = New-Tree 'skip' $last @('bin/ild_fixes_added.txt') $updater
    $null = New-Full 'skip' $a $base
    $null = New-Full 'skip' $b $next
    $null = New-Full 'skip' $c $last
    $null = New-Patch 'skip' $b $a $next $base
    $null = New-Patch 'skip' $c $b $last $next
    $game = New-Install 'skip' $a
    $status = Invoke-Update 'skip' $game
    Assert-That ($status.state -eq 'ready' -and $status.version -eq $last -and $status.patch -eq '0') ("$base skips " +
        "$next and downloads the full $last, state=$($status.state) version=$($status.version) patch=$($status.patch)")
    Wait-Restart $game
    Assert-That ((Get-InstalledVersion $game) -eq $last) "the installation reports $last"
    Assert-That (Test-File $game 'bin\ild_fixes_added.txt') 'a file first shipped later is installed'
    Assert-That (-not (Test-File $game 'bin\ild_fixes_dropped.txt')) 'a file no longer shipped is removed'
    Assert-That (-not (Test-File $game 'bin\ild_fixes_intermediate.txt')) 'nothing of the skipped release lands'
    $managed = '.ild-fixes\managed-files.txt'
    $owned = (Read-Text (Join-Path $game $managed)) -eq (Read-Text (Join-Path $c $managed))
    Assert-That $owned "the ownership list is the one $last ships"
    $untouched = (Read-Text (Join-Path $game 'gamedata\scripts\_g.script')) -eq 'original mod' -and
        (Read-Text (Join-Path $game 'appdata\savedgames\all.sav')) -eq 'original save'
    Assert-That $untouched 'original files and saves are untouched'

    # Past a withdrawn release: the newest patch was cut against it, so the helper takes the full archive instead,
    # before the game closes.
    $a = New-Tree 'withdrawn' $base @() $updater
    $b = New-Tree 'withdrawn' $next @() $updater
    $c = New-Tree 'withdrawn' $last @() $updater
    $null = New-Full 'withdrawn' $a $base
    $full = New-Full 'withdrawn' $c $last
    $null = New-Patch 'withdrawn' $c $b $last $next
    $game = New-Install 'withdrawn' $a
    $api = Start-Mock 'withdrawn'
    try {
        $helper = Start-Helper $game $PID '--qa-download'
        Wait-Until { $helper.HasExited } 'the withdrawn download' 60
    }
    finally { Stop-Process -Id $api.Id -Force -ErrorAction SilentlyContinue }
    $status = Read-Status $game $PID
    Assert-That ($helper.ExitCode -eq 0 -and $status.state -eq 'ready') ("the download past the withdrawn $next is " +
        "ready, exit=$($helper.ExitCode) state=$($status.state) $($status.error)")
    Assert-That ($status.patch -eq '0' -and $status.total -eq "$((Get-Item -LiteralPath $full).Length)") ("the patch " +
        "cut against $next is swapped for the full archive, patch=$($status.patch) total=$($status.total)")
    $rejected = (Read-Text (Join-Path $game '.ild-fixes\patch-rejected.txt')).Trim()
    Assert-That ($rejected -eq $last) 'the swap is remembered for the next session'
    $cached = Join-Path $game ".ild-fixes\update-cache\$last\$(Split-Path -Leaf $full)"
    Assert-That (Test-Path -LiteralPath $cached) 'the full archive waits in the cache'

    # A patch cut against the installed version is kept. It leaves the unchanged updater out, so the installed one
    # applies it.
    $a = New-Tree 'patch' $base @() $updater
    $b = New-Tree 'patch' $next @('bin/ild_fixes_added.txt') $updater
    $null = New-Full 'patch' $a $base
    $null = New-Full 'patch' $b $next
    $patch = New-Patch 'patch' $b $a $next $base
    Assert-That (-not (Test-Entry $patch 'InTheLineOfDutyFixesUpdater.exe')) 'the patch omits the unchanged updater'
    $game = New-Install 'patch' $a
    $status = Invoke-Update 'patch' $game
    Assert-That ($status.state -eq 'ready' -and $status.patch -eq '1') ("the patch cut against $base is kept, " +
        "state=$($status.state) patch=$($status.patch) $($status.error)")
    Wait-Restart $game
    Assert-That ((Get-InstalledVersion $game) -eq $next) "the patched installation reports $next"
    Assert-That (Test-File $game 'bin\ild_fixes_added.txt') 'the patch adds its file'

    # The updater inside the target archive is the one started to apply it. Here it is a stub that only records how
    # it was called, so nothing the installed updater does can pass for it.
    $a = New-Tree 'carried' $base @() $updater
    $b = New-Tree 'carried' $next @() $stub
    $null = New-Full 'carried' $a $base
    $null = New-Full 'carried' $b $next
    $game = New-Install 'carried' $a
    $status = Invoke-Update 'carried' $game
    Assert-That ($status.state -eq 'ready') "the archive is ready, state=$($status.state) $($status.error)"
    $proof = Join-Path $game 'restart-proof.txt'
    Wait-Until { Test-Path -LiteralPath $proof } 'the carried updater to start'
    $pattern = '--apply .*update-cache\\' + [regex]::Escape($next) + '\\'
    Assert-That ((Read-Text $proof) -match $pattern) 'the updater carried by the archive is started to apply it'
}
finally {
    Get-Process | Where-Object { $_.Path -and $_.Path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) } |
        Stop-Process -Force -ErrorAction SilentlyContinue
}
Write-Output "PASS $script:checks checks. Artifacts: $root"
exit 0
