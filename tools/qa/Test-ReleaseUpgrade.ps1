[CmdletBinding()]
param(
    [string]$Version,
    [int]$Oldest = 4,
    # 1.0.5 to 1.0.7 shipped a payload path their own updater does not own. Those installations recorded it in
    # their ownership manifest, so their updater now rejects its own list before it looks at any archive: no
    # release can reach them and the player has to unpack one by hand once. They are pinned here so the rest of
    # the run stays meaningful, and they drop out of it on their own as the window moves past them.
    [string[]]$Stranded = @('1.0.5', '1.0.6', '1.0.7')
)

# The helper that validates a release archive is the one the player already has, not the one just built. A payload
# path an older helper does not own is rejected whole, the player keeps the version they are on, and nothing in a
# build of the current tree can show it. This applies the candidate archives with each previous release's own
# updater, on an installation unpacked from that release.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$stub = Join-Path $repo 'build\Release\ild_restart_stub.exe'
if (-not (Test-Path -LiteralPath $stub)) { throw 'Build the ild_restart_stub target first.' }
if (-not $Version) {
    $Version = [regex]::Match((Get-Content -LiteralPath (Join-Path $repo 'CMakeLists.txt') -Raw),
        'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
}
$artifacts = Join-Path $repo 'artifacts\candidate'
$candidate = Join-Path $artifacts "In-the-line-of-duty-fixes-$Version-Setup_Manual.zip"
if (-not (Test-Path -LiteralPath $candidate)) { throw "Package the candidate first: $candidate" }

$root = Join-Path $repo ('qa\upgrade-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$script:checks = 0

function Assert-That([bool]$Condition, [string]$Message) {
    ++$script:checks
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}

$bases = @(Get-ChildItem -LiteralPath $artifacts -Filter 'In-the-line-of-duty-fixes-*-Setup_Manual.zip' |
    ForEach-Object {
        $found = [regex]::Match($_.Name, '-([0-9]+\.[0-9]+\.[0-9]+)-Setup_Manual\.zip$')
        if ($found.Success) { [pscustomobject]@{ Version = [version]$found.Groups[1].Value; Path = $_.FullName } }
    } | Where-Object { $_.Version -lt [version]$Version } | Sort-Object Version -Descending |
    Select-Object -First $Oldest)
if (-not $bases) { throw 'No previous release archive to upgrade from.' }

foreach ($base in $bases) {
    $game = Join-Path $root "game-$($base.Version)"
    [IO.Compression.ZipFile]::ExtractToDirectory($base.Path, $game)
    Copy-Item -LiteralPath $stub -Destination (Join-Path $game 'bin\XR_3DA.exe') -Force
    # The engine the mod ships is not here, so an untouched original is represented by a file the pack never owns.
    $foreign = Join-Path $game 'gamedata\scripts\_g.script'
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $foreign) | Out-Null
    [IO.File]::WriteAllText($foreign, 'original mod')

    $cache = Join-Path $game ".ild-fixes\update-cache\$Version"
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $staged = Join-Path $cache 'payload.zip'
    Copy-Item -LiteralPath $candidate -Destination $staged -Force
    # The installed helper is copied aside the way the running game copies it, and it is the one under test.
    $runner = Join-Path $cache 'runner.exe'
    Copy-Item -LiteralPath (Join-Path $game 'InTheLineOfDutyFixesUpdater.exe') -Destination $runner -Force
    $digest = 'sha256:' + (Get-FileHash -LiteralPath $staged).Hash.ToLowerInvariant()
    $arguments = '--apply --quiet --game-dir "{0}" --archive "{1}" --version {4} --digest {2} --size {3} --wait-pid 0 --restart-exe "{0}\bin\XR_3DA.exe" --restart-args ""' -f
        $game, $staged, $digest, (Get-Item -LiteralPath $staged).Length, $Version
    $process = Start-Process -FilePath $runner -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru

    $reason = ''
    $result = Join-Path $game '.ild-fixes\runtime\apply-result.txt'
    if (Test-Path -LiteralPath $result) { $reason = (Get-Content -LiteralPath $result -Raw).Trim() -replace "`r?`n", ' | ' }
    if ("$($base.Version)" -in $Stranded) {
        Assert-That ($process.ExitCode -ne 0) "$($base.Version) is stranded and still refuses the archive, exit=$($process.ExitCode) $reason"
        Assert-That ((Get-Content -LiteralPath (Join-Path $game '.ild-fixes\version.txt')).Trim() -eq "$($base.Version)") "$($base.Version) keeps the version it had"
    } else {
        Assert-That ($process.ExitCode -eq 0) "$($base.Version) -> $Version applies with the $($base.Version) updater, exit=$($process.ExitCode) $reason"
        Assert-That ((Get-Content -LiteralPath (Join-Path $game '.ild-fixes\version.txt')).Trim() -eq $Version) "$($base.Version) install reports $Version afterwards"
    }
    Assert-That ((Get-Content -LiteralPath $foreign -Raw) -eq 'original mod') "$($base.Version) upgrade leaves an unowned file alone"
}
Write-Output "PASS $script:checks checks. Artifacts: $root"
exit 0
