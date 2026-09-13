[CmdletBinding()]
param(
    [string]$Version,
    # Every earlier release is one a client may still be sitting on; a number keeps only the latest ones.
    [int]$Oldest = [int]::MaxValue,
    # 1.0.5 to 1.0.7 shipped a payload path their own updater does not own. Those installations recorded it in
    # their ownership manifest, so their updater rejects its own list before it changes anything: no release can
    # reach them through it and the player has to unpack one by hand once. They are pinned here as refusing, so the
    # rest of the run stays meaningful and nothing passes them off as rescued.
    [string[]]$Stranded = @('1.0.5', '1.0.6', '1.0.7'),
    [string]$Artifacts = (Join-Path $PSScriptRoot '..\..\artifacts\candidate'),
    [string]$RestartStub = (Join-Path $PSScriptRoot '..\..\build\Release\ild_restart_stub.exe')
)

# The helper that validates a release archive is the one the player already has, not the one just built. A payload
# path an older helper does not own is rejected whole, the player keeps the version they are on, and nothing in a
# build of the current tree can show it. This applies the candidate archive with each previous release's own
# updater, on an installation unpacked from that release.
# A helper that hands the archive to the updater inside it leaves the older installation to the candidate's rules
# instead, so every base is upgraded that way too, stranded ones included: a path any release has shipped must stay
# owned by every later updater.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not (Test-Path -LiteralPath $RestartStub)) { throw 'Build the ild_restart_stub target first.' }
$stub = (Resolve-Path -LiteralPath $RestartStub).Path
if (-not $Version) {
    $Version = [regex]::Match((Get-Content -LiteralPath (Join-Path $repo 'CMakeLists.txt') -Raw),
        'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
}
$artifacts = (Resolve-Path -LiteralPath $Artifacts).Path
$candidate = Join-Path $artifacts "In-the-line-of-duty-fixes-$Version-Setup_Manual.zip"
if (-not (Test-Path -LiteralPath $candidate)) { throw "Package the candidate first: $candidate" }

$root = Join-Path $repo ('qa\upgrade-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$script:checks = 0

function Assert-That([bool]$Condition, [string]$Message) {
    ++$script:checks
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}

function Install-Base([string]$Archive, [string]$Name) {
    $game = Join-Path $root $Name
    [IO.Compression.ZipFile]::ExtractToDirectory($Archive, $game)
    Copy-Item -LiteralPath $stub -Destination (Join-Path $game 'bin\XR_3DA.exe') -Force
    # The engine the mod ships is not here, so an untouched original is represented by a file the pack never owns.
    $foreign = Join-Path $game 'gamedata\scripts\_g.script'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $foreign) | Out-Null
    [IO.File]::WriteAllText($foreign, 'original mod')
    return $game
}

# The runner is copied aside the way the running game copies it, and it is the one under test.
function Invoke-Upgrade([string]$Game, [string]$Runner) {
    $cache = Join-Path $Game ".ild-fixes\update-cache\$Version"
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $staged = Join-Path $cache 'payload.zip'
    Copy-Item -LiteralPath $candidate -Destination $staged -Force
    $cached = Join-Path $cache 'runner.exe'
    Copy-Item -LiteralPath $Runner -Destination $cached -Force
    $digest = 'sha256:' + (Get-FileHash -LiteralPath $staged).Hash.ToLowerInvariant()
    $arguments = '--apply --quiet --game-dir "{0}" --archive "{1}" --version {4} --digest {2} --size {3} --wait-pid 0 --restart-exe "{0}\bin\XR_3DA.exe" --restart-args ""' -f
        $Game, $staged, $digest, (Get-Item -LiteralPath $staged).Length, $Version
    $process = Start-Process -FilePath $cached -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $reason = ''
    $result = Join-Path $Game '.ild-fixes\runtime\apply-result.txt'
    if (Test-Path -LiteralPath $result) { $reason = (Get-Content -LiteralPath $result -Raw).Trim() -replace "`r?`n", ' | ' }
    [pscustomobject]@{
        Code = $process.ExitCode
        Reason = $reason
        Version = (Get-Content -LiteralPath (Join-Path $Game '.ild-fixes\version.txt')).Trim()
        Untouched = (Get-Content -LiteralPath (Join-Path $Game 'gamedata\scripts\_g.script') -Raw) -eq 'original mod'
    }
}

$carried = Join-Path $root 'candidate-updater.exe'
$zip = [IO.Compression.ZipFile]::OpenRead($candidate)
try { [IO.Compression.ZipFileExtensions]::ExtractToFile($zip.GetEntry('InTheLineOfDutyFixesUpdater.exe'), $carried) }
finally { $zip.Dispose() }

$bases = @(Get-ChildItem -LiteralPath $artifacts -Filter 'In-the-line-of-duty-fixes-*-Setup_Manual.zip' |
    ForEach-Object {
        $found = [regex]::Match($_.Name, '-([0-9]+\.[0-9]+\.[0-9]+)-Setup_Manual\.zip$')
        if ($found.Success) { [pscustomobject]@{ Version = [version]$found.Groups[1].Value; Path = $_.FullName } }
    } | Where-Object { $_.Version -lt [version]$Version } | Sort-Object Version -Descending |
    Select-Object -First $Oldest)
if (-not $bases) { throw 'No previous release archive to upgrade from.' }

foreach ($base in $bases) {
    $name = "$($base.Version)"
    $game = Install-Base $base.Path "game-$name"
    $own = Invoke-Upgrade $game (Join-Path $game 'InTheLineOfDutyFixesUpdater.exe')
    $outcome = "exit=$($own.Code) $($own.Reason)"
    if ($name -in $Stranded) {
        Assert-That ($own.Code -ne 0) "$name is stranded and still refuses the archive, $outcome"
        Assert-That ($own.Version -eq $name) "$name keeps the version it had"
    } else {
        Assert-That ($own.Code -eq 0) "$name -> $Version applies with the $name updater, $outcome"
        Assert-That ($own.Version -eq $Version) "$name install reports $Version afterwards"
    }
    Assert-That $own.Untouched "$name upgrade leaves an unowned file alone"

    $handed = Invoke-Upgrade (Install-Base $base.Path "carried-$name") $carried
    $outcome = "exit=$($handed.Code) $($handed.Reason)"
    Assert-That ($handed.Code -eq 0) "$name -> $Version applies with the updater the candidate carries, $outcome"
    Assert-That ($handed.Version -eq $Version) "$name install reports $Version after the carried updater"
    Assert-That $handed.Untouched "$name carried upgrade leaves an unowned file alone"
}
Write-Output "PASS $script:checks checks. Artifacts: $root"
exit 0
