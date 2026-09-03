[CmdletBinding()]
param(
    [string]$GameRoot = 'D:\Games\S.T.A.L.K.E.R',
    [switch]$Interactive
)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$GameRoot = [IO.Path]::GetFullPath($GameRoot).TrimEnd('\')
$version = [regex]::Match((Get-Content "$repo\CMakeLists.txt" -Raw),
    'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
$payload = Join-Path $repo "artifacts\candidate\payload-$version"
$manifestPath = '.ild-fixes/update-manifest.txt'
# Every candidate file must be byte-identical to the current build output or source; stale candidates never ship.
$sourceMap = @{
    'bin/dinput8.dll' = 'build\Release\dinput8.dll'
    'InTheLineOfDutyFixesUpdater.exe' = 'build\updater\InTheLineOfDutyFixesUpdater.exe'
    'gamedata/scripts/ild_fix_ui.script' = 'payload\gamedata\scripts\ild_fix_ui.script'
    'gamedata/scripts/ild_gameplay.script' = 'payload\gamedata\scripts\ild_gameplay.script'
    'gamedata/scripts/ild_script_repairs.script' = 'payload\gamedata\scripts\ild_script_repairs.script'
    'gamedata/scripts/ild_recipe_repairs.script' = 'payload\gamedata\scripts\ild_recipe_repairs.script'
    'gamedata/config/ui/ild_fixes_update.xml' = 'payload\gamedata\config\ui\ild_fixes_update.xml'
    'gamedata/config/ui/ild_fixes_options.xml' = 'payload\gamedata\config\ui\ild_fixes_options.xml'
    'README-InTheLineOfDutyFixes.txt' = 'packaging\README-InTheLineOfDutyFixes.txt'
}
$expected = @(@($sourceMap.Keys) + @('.ild-fixes/version.txt', '.ild-fixes/managed-files.txt', $manifestPath) | Sort-Object)

function Save-Snapshot {
    Get-ChildItem 'C:\Users\Public\Documents\STALKER-SHOC\savedgames' -Recurse -File |
        Sort-Object FullName | ForEach-Object { "$($_.FullName)`t$((Get-FileHash $_.FullName).Hash)" }
}

function Assert-NoReparse([string]$Path) {
    # The game root itself may be a junction (a relocated library); only components below it are checked.
    $current = $Path
    while ($current -and $current.Length -gt $GameRoot.Length) {
        if (Test-Path -LiteralPath $current) {
            if ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse point is not a deployment target: $current"
            }
        }
        $current = Split-Path -Parent $current
    }
}

function Copy-Atomic([string]$Source, [string]$Target) {
    $temporary = "$Target.ild-deploy-tmp"
    Copy-Item -LiteralPath $Source -Destination $temporary -Force
    Move-Item -LiteralPath $temporary -Destination $Target -Force
}

try {
    if (Get-Process -Name XR_3DA, InTheLineOfDutyFixesUpdater, IldFixesUpdater.cached -ErrorAction SilentlyContinue) {
        throw 'Close the game and updater before deploying.'
    }
    if (-not (Test-Path "$GameRoot\bin\XR_3DA.exe" -PathType Leaf)) { throw 'Game root is invalid.' }
    if (-not (Test-Path -LiteralPath $payload)) {
        throw "Candidate payload is missing: $payload. Run tools\package\Build-Package.ps1 first."
    }
    foreach ($relative in $sourceMap.Keys) {
        $built = Join-Path $repo $sourceMap[$relative]
        if (-not (Test-Path -LiteralPath $built -PathType Leaf)) { throw "Build output is missing: $built" }
        if ((Get-FileHash -LiteralPath $built).Hash -ne (Get-FileHash -LiteralPath (Join-Path $payload $relative)).Hash) {
            throw "Stale candidate: $relative differs from the current build. Re-run Build-Package.ps1."
        }
    }
    $managed = @(Get-Content "$payload\.ild-fixes\managed-files.txt" | Sort-Object)
    if (Compare-Object $expected $managed) { throw 'Unexpected payload ownership manifest.' }
    $ownedPath = "$GameRoot\.ild-fixes\managed-files.txt"
    $owned = if (Test-Path $ownedPath) { @(Get-Content $ownedPath) } else { @() }
    $entries = @{}
    foreach ($line in (Get-Content "$payload\$manifestPath" | Select-Object -Skip 2)) {
        $parts = $line.Split("`t")
        if ($parts.Count -ne 3 -or $parts[2] -notin $expected -or $parts[2] -eq $manifestPath -or
            $entries.ContainsKey($parts[2])) {
            throw 'Invalid payload manifest entry.'
        }
        $entries[$parts[2]] = $parts
    }
    if ($entries.Count -ne $expected.Count - 1) { throw 'Incomplete payload manifest.' }
    foreach ($relative in $expected) {
        $source = Join-Path $payload $relative
        $target = Join-Path $GameRoot $relative
        Assert-NoReparse $target
        if ((Test-Path $target) -and $relative -notin $owned) { throw "Foreign file collision: $target" }
        if ($relative -ne $manifestPath -and ((Get-FileHash $source).Hash -ne $entries[$relative][0] -or
            (Get-Item $source).Length -ne [long]$entries[$relative][1])) { throw "Invalid payload: $relative" }
    }
    if ((Get-Content "$payload\.ild-fixes\version.txt" -Raw).Trim() -ne $version) { throw 'Wrong payload version.' }
    $savesBefore = @(Save-Snapshot)
    $backup = Join-Path $repo ('qa\deploy-backup-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $backup | Out-Null
    foreach ($relative in $expected) {
        $target = Join-Path $GameRoot $relative
        if (Test-Path $target) {
            $copy = Join-Path $backup $relative
            New-Item -ItemType Directory -Force -Path (Split-Path $copy) | Out-Null
            Copy-Item -LiteralPath $target -Destination $copy
        }
    }
    $changed = [Collections.Generic.List[string]]::new()
    try {
        foreach ($relative in $expected) {
            $source = Join-Path $payload $relative
            $target = Join-Path $GameRoot $relative
            New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
            $changed.Add($relative)
            Copy-Atomic $source $target
            if ((Get-FileHash $target).Hash -ne (Get-FileHash $source).Hash) { throw "Deployment mismatch: $relative" }
        }
    }
    catch {
        $failure = $_
        foreach ($relative in $changed) {
            try {
                $target = Join-Path $GameRoot $relative
                $copy = Join-Path $backup $relative
                if (Test-Path $copy) { Copy-Atomic $copy $target }
                elseif (Test-Path $target) { Remove-Item -LiteralPath $target -Force }
            }
            catch { Write-Warning "Rollback failed for ${relative}: $_" }
        }
        throw $failure
    }
    if (Compare-Object $savesBefore @(Save-Snapshot)) { throw 'Save tree changed during deployment.' }
    Write-Output "Deployed $version to $GameRoot; all $($expected.Count) runtime hashes verified; saves unchanged."
    Write-Output "Recoverable backup: $backup"
}
catch {
    Write-Error $_ -ErrorAction Continue
    if ($Interactive) { Read-Host 'Press Enter to close' | Out-Null }
    exit 1
}
if ($Interactive) { Read-Host 'Press Enter to close' | Out-Null }
