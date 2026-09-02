[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$root = Join-Path $repo ('qa\update-flow-' + [Guid]::NewGuid().ToString('N'))
$updater = Join-Path $repo 'build\updater\InTheLineOfDutyFixesUpdater.exe'
$stub = Join-Path $repo 'build\Release\ild_restart_stub.exe'
if (-not (Test-Path -LiteralPath $updater) -or -not (Test-Path -LiteralPath $stub)) {
    throw 'Build the updater and ild_restart_stub targets first.'
}
New-Item -ItemType Directory -Path $root | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$utf8 = [Text.UTF8Encoding]::new($false)
$script:checks = 0

function Assert-That([bool]$Condition, [string]$Message) {
    ++$script:checks
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}

function Write-Text([string]$Path, [string]$Text) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    [IO.File]::WriteAllText($Path, $Text, $utf8)
}

function New-Version([string]$Path, [string]$Version, [bool]$Target) {
    New-Item -ItemType Directory -Force -Path (Join-Path $Path 'bin') | Out-Null
    Copy-Item -LiteralPath $updater -Destination (Join-Path $Path 'InTheLineOfDutyFixesUpdater.exe')
    Write-Text (Join-Path $Path 'bin\dinput8.dll') "loader-$Version"
    Write-Text (Join-Path $Path 'bin\ild_fixes_unchanged.txt') 'unchanged'
    Write-Text (Join-Path $Path 'bin\ild_fixes_changed.txt') "changed-$Version"
    if ($Target) { Write-Text (Join-Path $Path 'bin\ild_fixes_added.txt') 'added' }
    else { Write-Text (Join-Path $Path 'bin\ild_fixes_dropped.txt') 'dropped' }
    Write-Text (Join-Path $Path '.ild-fixes\version.txt') "$Version`n"
    $files = @(Get-ChildItem -LiteralPath $Path -File -Recurse | ForEach-Object {
        $_.FullName.Substring($Path.Length + 1).Replace('\', '/')
    }) + '.ild-fixes/managed-files.txt'
    Write-Text (Join-Path $Path '.ild-fixes\managed-files.txt') (($files | Sort-Object) -join "`n")
    $files
}

function New-Install([string]$Name) {
    $game = Join-Path $root $Name
    $null = New-Version $game '0.1.0' $false
    Copy-Item -LiteralPath $stub -Destination (Join-Path $game 'bin\XR_3DA.exe')
    Write-Text (Join-Path $game 'bin\xrCore.dll') 'original engine'
    Write-Text (Join-Path $game 'gamedata\scripts\_g.script') 'original mod'
    Write-Text (Join-Path $game 'appdata\savedgames\all.sav') 'original save'
    return $game
}

function Snapshot([string]$Game) {
    @(Get-ChildItem -LiteralPath $Game -Recurse -File | Where-Object {
        $_.FullName -notlike '*\.ild-fixes\update-cache\*' -and $_.Name -ne 'restart-proof.txt' -and
        $_.Name -ne 'patch-rejected.txt'
    } | ForEach-Object {
        $_.FullName.Substring($Game.Length + 1) + '=' + (Get-FileHash -LiteralPath $_.FullName).Hash
    } | Sort-Object) -join "`n"
}

function Invoke-Apply([string]$Game, [string]$Archive, [string]$Digest = '') {
    $cache = Join-Path $Game '.ild-fixes\update-cache\0.1.1'
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $staged = Join-Path $cache 'payload.zip'
    Copy-Item -LiteralPath $Archive -Destination $staged -Force
    $runner = Join-Path $cache 'runner.exe'
    Copy-Item -LiteralPath $updater -Destination $runner -Force
    if (-not $Digest) { $Digest = 'sha256:' + (Get-FileHash -LiteralPath $staged).Hash.ToLowerInvariant() }
    $arguments = '--apply --game-dir "{0}" --archive "{1}" --version 0.1.1 --digest {2} --size {3} --wait-pid 0 --restart-exe "{0}\bin\XR_3DA.exe" --restart-args ""' -f
        $Game, $staged, $Digest, (Get-Item -LiteralPath $staged).Length
    $process = Start-Process -FilePath $runner -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -eq 0) {
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        while (-not (Test-Path -LiteralPath (Join-Path $Game 'bin\restart-proof.txt')) -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
        }
        Assert-That (Test-Path -LiteralPath (Join-Path $Game 'bin\restart-proof.txt')) 'restart completed'
    }
    return $process.ExitCode
}

$target = Join-Path $root 'target'
$targetFiles = @(New-Version $target '0.1.1' $true)
$rows = foreach ($relative in ($targetFiles | Sort-Object)) {
    $file = Get-Item -LiteralPath (Join-Path $target $relative)
    '{0}{1}{2}{1}{3}' -f (Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant(), "`t", $file.Length, $relative
}
Write-Text (Join-Path $target 'update-manifest.txt') ((@('schema=ild-fixes.update/1','version=0.1.1') + $rows) -join "`n")
$full = Join-Path $root 'full.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($target, $full)
$patchRoot = Join-Path $root 'patch'
Copy-Item -LiteralPath $target -Destination $patchRoot -Recurse
Write-Text (Join-Path $patchRoot 'update-manifest.txt') ((@('schema=ild-fixes.update/2','version=0.1.1','kind=patch','base=0.1.0') + $rows) -join "`n")
foreach ($relative in @('InTheLineOfDutyFixesUpdater.exe','bin\ild_fixes_unchanged.txt')) {
    [IO.File]::Delete((Join-Path $patchRoot $relative))
}
$patch = Join-Path $root 'patch.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($patchRoot, $patch)

$fullGame = New-Install 'full-game'
$code = Invoke-Apply $fullGame $full
Assert-That ($code -eq 0) "full update exit=$code"
$patchGame = New-Install 'patch-game'
$code = Invoke-Apply $patchGame $patch
Assert-That ($code -eq 0) "patch update exit=$code"
Assert-That ((Snapshot $fullGame) -eq (Snapshot $patchGame)) 'full and patch results are identical'
Assert-That ((Get-Content -LiteralPath (Join-Path $fullGame 'gamedata\scripts\_g.script') -Raw) -eq 'original mod') 'original mod preserved'
Assert-That ((Get-Content -LiteralPath (Join-Path $fullGame 'appdata\savedgames\all.sav') -Raw) -eq 'original save') 'save preserved'

$dirty = New-Install 'dirty-game'
Write-Text (Join-Path $dirty 'bin\ild_fixes_unchanged.txt') 'modified'
$before = Snapshot $dirty
$code = Invoke-Apply $dirty $patch
Assert-That ($code -eq 24) "inapplicable patch rejected, exit=$code"
Assert-That ((Snapshot $dirty) -eq $before) 'patch rejection leaves installation unchanged'
$code = Invoke-Apply $dirty $full
Assert-That ($code -eq 0) 'full archive applies after patch rejection'

$corrupt = New-Install 'corrupt-game'
$before = Snapshot $corrupt
$code = Invoke-Apply $corrupt $full ('sha256:' + ('0' * 64))
Assert-That ($code -eq 20) 'incorrect archive digest rejected'
Assert-That ((Snapshot $corrupt) -eq $before) 'hash rejection leaves installation unchanged'

$collision = New-Install 'collision-game'
Write-Text (Join-Path $collision 'bin\ild_fixes_added.txt') 'foreign file'
$before = Snapshot $collision
$code = Invoke-Apply $collision $full
Assert-That ($code -eq 26) 'foreign destination collision rejected'
Assert-That ((Snapshot $collision) -eq $before) 'foreign file is not overwritten'

$rollback = New-Install 'rollback-game'
$before = Snapshot $rollback
$lock = [IO.File]::Open((Join-Path $rollback 'bin\dinput8.dll'), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try { $code = Invoke-Apply $rollback $full } finally { $lock.Dispose() }
Assert-That ($code -eq 22) "failed replacement rolled back, exit=$code"
Assert-That ((Snapshot $rollback) -eq $before) 'rollback restored complete prior file set'
Write-Output "PASS $script:checks checks. Artifacts: $root"
exit 0
