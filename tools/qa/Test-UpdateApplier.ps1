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
$manifestPath = '.ild-fixes/update-manifest.txt'
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

function Get-Rows([string]$Path, [string[]]$Files) {
    foreach ($relative in ($Files | Sort-Object)) {
        if ($relative -eq $manifestPath) { continue }
        $file = Get-Item -LiteralPath (Join-Path $Path $relative)
        '{0}{1}{2}{1}{3}' -f (Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant(), "`t", $file.Length, $relative
    }
}

function Write-Manifest([string]$Path, [string[]]$Header, [string[]]$Rows) {
    Write-Text (Join-Path $Path '.ild-fixes\update-manifest.txt') (($Header + $Rows) -join "`n")
}

function New-Version([string]$Path, [string]$Version, [bool]$Target, [bool]$WithManifest) {
    New-Item -ItemType Directory -Force -Path (Join-Path $Path 'bin') | Out-Null
    Copy-Item -LiteralPath $updater -Destination (Join-Path $Path 'InTheLineOfDutyFixesUpdater.exe')
    Write-Text (Join-Path $Path 'bin\dinput8.dll') "loader-$Version"
    Write-Text (Join-Path $Path 'bin\ild_fixes_unchanged.txt') 'unchanged'
    Write-Text (Join-Path $Path 'bin\ild_fixes_changed.txt') "changed-$Version"
    if ($Target) {
        Write-Text (Join-Path $Path 'gamedata\scripts\ild_gameplay.script') 'gameplay-fixes'
        Write-Text (Join-Path $Path 'gamedata\scripts\ild_script_repairs.script') 'script-repairs'
        Write-Text (Join-Path $Path 'gamedata\scripts\ild_recipe_repairs.script') 'recipe-repairs'
        Write-Text (Join-Path $Path 'bin\ild_fixes_added.txt') 'added'
    } else {
        Write-Text (Join-Path $Path 'bin\ild_fixes_dropped.txt') 'dropped'
    }
    Write-Text (Join-Path $Path '.ild-fixes\version.txt') "$Version`n"
    $files = @(Get-ChildItem -LiteralPath $Path -File -Recurse | ForEach-Object {
        $_.FullName.Substring($Path.Length + 1).Replace('\', '/')
    }) + '.ild-fixes/managed-files.txt'
    if ($WithManifest) { $files += $manifestPath }
    Write-Text (Join-Path $Path '.ild-fixes\managed-files.txt') (($files | Sort-Object) -join "`n")
    if ($WithManifest) { Write-Manifest $Path @('schema=ild-fixes.update/1', "version=$Version") @(Get-Rows $Path $files) }
    $files
}

function New-Install([string]$Name, [bool]$WithManifest = $false) {
    $game = Join-Path $root $Name
    $null = New-Version $game '0.1.0' $false $WithManifest
    Copy-Item -LiteralPath $stub -Destination (Join-Path $game 'bin\XR_3DA.exe')
    Write-Text (Join-Path $game 'bin\xrCore.dll') 'original engine'
    Write-Text (Join-Path $game 'gamedata\scripts\_g.script') 'original mod'
    Write-Text (Join-Path $game 'appdata\savedgames\all.sav') 'original save'
    return $game
}

function Snapshot([string]$Game) {
    @(Get-ChildItem -LiteralPath $Game -Recurse -File | Where-Object {
        $_.FullName -notlike '*\.ild-fixes\update-cache\*' -and $_.FullName -notlike '*\.ild-fixes\runtime\*' -and
        $_.Name -ne 'restart-proof.txt' -and $_.Name -ne 'patch-rejected.txt'
    } | ForEach-Object {
        $_.FullName.Substring($Game.Length + 1) + '=' + (Get-FileHash -LiteralPath $_.FullName).Hash
    } | Sort-Object) -join "`n"
}

function New-Archive([string]$Source, [string]$Name) {
    $archive = Join-Path $root $Name
    [IO.Compression.ZipFile]::CreateFromDirectory($Source, $archive)
    return $archive
}

function Copy-Target([string]$Name) {
    $copy = Join-Path $root $Name
    Copy-Item -LiteralPath $target -Destination $copy -Recurse
    return $copy
}

function Invoke-Apply([string]$Game, [string]$Archive, [string]$Digest = '', [string]$RestartArgs = '',
    [string]$Version = '0.1.1') {
    $cache = Join-Path $Game ".ild-fixes\update-cache\$Version"
    New-Item -ItemType Directory -Force -Path $cache | Out-Null
    $staged = Join-Path $cache 'payload.zip'
    Copy-Item -LiteralPath $Archive -Destination $staged -Force
    $runner = Join-Path $cache 'runner.exe'
    Copy-Item -LiteralPath $updater -Destination $runner -Force
    if (-not $Digest) { $Digest = 'sha256:' + (Get-FileHash -LiteralPath $staged).Hash.ToLowerInvariant() }
    Remove-Item -LiteralPath (Join-Path $Game 'bin\restart-proof.txt') -Force -ErrorAction SilentlyContinue
    $arguments = '--apply --quiet --game-dir "{0}" --archive "{1}" --version {4} --digest {2} --size {3} --wait-pid 0 --restart-exe "{0}\bin\XR_3DA.exe" --restart-args "{5}"' -f
        $Game, $staged, $Digest, (Get-Item -LiteralPath $staged).Length, $Version, $RestartArgs
    $process = Start-Process -FilePath $runner -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while (-not (Test-Path -LiteralPath (Join-Path $Game 'bin\restart-proof.txt')) -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 100
    }
    return $process.ExitCode
}

function Get-RestartProof([string]$Game) {
    $path = Join-Path $Game 'bin\restart-proof.txt'
    if (Test-Path -LiteralPath $path) { return Get-Content -LiteralPath $path -Raw }
    return ''
}

function Get-ApplyResult([string]$Game) {
    $path = Join-Path $Game '.ild-fixes\runtime\apply-result.txt'
    if (Test-Path -LiteralPath $path) { return Get-Content -LiteralPath $path -Raw }
    return ''
}

$target = Join-Path $root 'target'
$targetFiles = @(New-Version $target '0.1.1' $true $true)
$rows = @(Get-Rows $target $targetFiles)
$full = New-Archive $target 'full.zip'
$patchRoot = Copy-Target 'patch'
Write-Manifest $patchRoot @('schema=ild-fixes.update/2', 'version=0.1.1', 'kind=patch', 'base=0.1.0') $rows
foreach ($relative in @('InTheLineOfDutyFixesUpdater.exe', 'bin\ild_fixes_unchanged.txt')) {
    [IO.File]::Delete((Join-Path $patchRoot $relative))
}
$patch = New-Archive $patchRoot 'patch.zip'

$fullGame = New-Install 'full-game'
$code = Invoke-Apply $fullGame $full '' '-start server(all/single/alife/new) client(localhost) -nointro'
Assert-That ($code -eq 0) "full update exit=$code"
$proof = Get-RestartProof $fullGame
Assert-That ($proof.StartsWith('restarted')) 'restart completed'
Assert-That ($proof.Contains('-start server(all/single/alife/new) client(localhost) -nointro')) 'raw restart arguments are preserved'
Assert-That (Test-Path -LiteralPath (Join-Path $fullGame '.ild-fixes\update-manifest.txt')) 'manifest is installed inside .ild-fixes'
Assert-That ((Get-Content -LiteralPath (Join-Path $fullGame '.ild-fixes\managed-files.txt')) -contains $manifestPath) 'manifest is an owned file'
Start-Sleep -Milliseconds 500
Assert-That (-not (Test-Path -LiteralPath (Join-Path $fullGame '.ild-fixes\update-cache'))) 'update cache is removed after a verified install'

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
Assert-That ((Get-ApplyResult $dirty).Contains('code=24')) 'patch rejection is recorded for the next session'
$code = Invoke-Apply $dirty $full
Assert-That ($code -eq 0) 'full archive applies after patch rejection on a legacy install'

$verified = New-Install 'verified-game' $true
Write-Text (Join-Path $verified 'bin\ild_fixes_unchanged.txt') 'modified'
$before = Snapshot $verified
$code = Invoke-Apply $verified $full
Assert-That ($code -eq 27) "modified fix-pack file blocks the update, exit=$code"
Assert-That ((Snapshot $verified) -eq $before) 'blocked update leaves installation unchanged'
Write-Text (Join-Path $verified 'bin\ild_fixes_unchanged.txt') 'unchanged'
$code = Invoke-Apply $verified $full
Assert-That ($code -eq 0) 'restored fix-pack file lets the update proceed'

$wrongBase = New-Install 'wrong-base-game' $true
Write-Text (Join-Path $wrongBase '.ild-fixes\version.txt') "0.0.9`n"
$before = Snapshot $wrongBase
$code = Invoke-Apply $wrongBase $patch
Assert-That ($code -eq 24) "patch for another base is rejected before mutation, exit=$code"
Assert-That ((Snapshot $wrongBase) -eq $before) 'base rejection leaves installation unchanged'

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
Assert-That ((Get-ApplyResult $collision).Contains('code=26')) 'collision is recorded for the next session'
Assert-That ((Get-RestartProof $collision).StartsWith('restarted')) 'game restarts after a rejected update'

$mismatch = New-Install 'version-mismatch-game'
$before = Snapshot $mismatch
$code = Invoke-Apply $mismatch $full '' '' '0.1.2'
Assert-That ($code -eq 21) "archive for another version rejected, exit=$code"
Assert-That ((Snapshot $mismatch) -eq $before) 'version mismatch leaves installation unchanged'

$traversalRoot = Copy-Target 'traversal'
Write-Manifest $traversalRoot @('schema=ild-fixes.update/1', 'version=0.1.1') ($rows + @("$('0' * 64)`t1`t../../evil.txt"))
$traversal = New-Install 'traversal-game'
$before = Snapshot $traversal
$code = Invoke-Apply $traversal (New-Archive $traversalRoot 'traversal.zip')
Assert-That ($code -eq 21) "path traversal in the manifest rejected, exit=$code"
Assert-That ((Snapshot $traversal) -eq $before) 'traversal leaves installation unchanged'

$aliasRoot = Copy-Target 'alias'
Write-Manifest $aliasRoot @('schema=ild-fixes.update/1', 'version=0.1.1') ($rows + @("$('0' * 64)`t1`tbin/ild_fixes_trailing.txt."))
$alias = New-Install 'alias-game'
$code = Invoke-Apply $alias (New-Archive $aliasRoot 'alias.zip')
Assert-That ($code -eq 21) "Windows path alias rejected, exit=$code"

$duplicateRoot = Copy-Target 'duplicate'
Write-Manifest $duplicateRoot @('schema=ild-fixes.update/1', 'version=0.1.1') ($rows + @($rows[0]))
$duplicate = New-Install 'duplicate-game'
$code = Invoke-Apply $duplicate (New-Archive $duplicateRoot 'duplicate.zip')
Assert-That ($code -eq 21) "duplicate manifest row rejected, exit=$code"

$foreignRoot = Copy-Target 'foreign'
Write-Text (Join-Path $foreignRoot 'gamedata\scripts\xr_logic.script') 'not ours'
$foreign = New-Install 'foreign-game'
$before = Snapshot $foreign
$code = Invoke-Apply $foreign (New-Archive $foreignRoot 'foreign.zip')
Assert-That ($code -eq 21) "archive with a non-fix-pack path rejected, exit=$code"
Assert-That ((Snapshot $foreign) -eq $before) 'foreign archive leaves installation unchanged'

$junctionTarget = New-Install 'junction-target'
$junction = Join-Path $root 'junction-game'
New-Item -ItemType Junction -Path $junction -Target $junctionTarget | Out-Null
$code = Invoke-Apply $junction $full
Assert-That ($code -eq 0) "junction game root is accepted, exit=$code"

$rollback = New-Install 'rollback-game'
$before = Snapshot $rollback
$lock = [IO.File]::Open((Join-Path $rollback 'bin\dinput8.dll'), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
try { $code = Invoke-Apply $rollback $full } finally { $lock.Dispose() }
Assert-That ($code -eq 22) "failed replacement rolled back, exit=$code"
Assert-That ((Snapshot $rollback) -eq $before) 'rollback restored complete prior file set'
Assert-That ((Get-ApplyResult $rollback).Contains('code=22')) 'rollback is recorded for the next session'
Write-Output "PASS $script:checks checks. Artifacts: $root"
exit 0
