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
$expected = @('bin/dinput8.dll', 'InTheLineOfDutyFixesUpdater.exe',
    'gamedata/scripts/ild_fix_ui.script', 'gamedata/scripts/ild_gameplay.script', 'gamedata/config/ui/ild_fixes_update.xml',
    'README-InTheLineOfDutyFixes.txt', '.ild-fixes/version.txt', '.ild-fixes/managed-files.txt') | Sort-Object

function Save-Snapshot {
    Get-ChildItem 'C:\Users\Public\Documents\STALKER-SHOC\savedgames' -Recurse -File |
        Sort-Object FullName | ForEach-Object { "$($_.FullName)`t$((Get-FileHash $_.FullName).Hash)" }
}

function Assert-NoReparse([string]$Path) {
    $current = $Path
    while ($current -and $current.Length -ge $GameRoot.Length) {
        if (Test-Path -LiteralPath $current) {
            if ((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse point is not a deployment target: $current"
            }
        }
        $current = Split-Path -Parent $current
    }
}

try {
    if (Get-Process -Name XR_3DA, InTheLineOfDutyFixesUpdater -ErrorAction SilentlyContinue) {
        throw 'Close the game and updater before deploying.'
    }
    if (-not (Test-Path "$GameRoot\bin\XR_3DA.exe" -PathType Leaf)) { throw 'Game root is invalid.' }
    $managed = @(Get-Content "$payload\.ild-fixes\managed-files.txt" | Sort-Object)
    if (Compare-Object $expected $managed) { throw 'Unexpected payload ownership manifest.' }
    $ownedPath = "$GameRoot\.ild-fixes\managed-files.txt"
    $owned = if (Test-Path $ownedPath) { @(Get-Content $ownedPath) } else { @() }
    $entries = @{}
    foreach ($line in (Get-Content "$payload\update-manifest.txt" | Select-Object -Skip 2)) {
        $parts = $line.Split("`t")
        if ($parts.Count -ne 3 -or $parts[2] -notin $expected -or $entries.ContainsKey($parts[2])) {
            throw 'Invalid payload manifest entry.'
        }
        $entries[$parts[2]] = $parts
    }
    if ($entries.Count -ne $expected.Count) { throw 'Incomplete payload manifest.' }
    foreach ($relative in $expected) {
        $source = Join-Path $payload $relative
        $target = Join-Path $GameRoot $relative
        Assert-NoReparse $target
        if ((Test-Path $target) -and $relative -notin $owned) { throw "Foreign file collision: $target" }
        if ((Get-FileHash $source).Hash -ne $entries[$relative][0] -or
            (Get-Item $source).Length -ne [long]$entries[$relative][1]) { throw "Invalid payload: $relative" }
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
            $target = Join-Path $GameRoot $relative
            New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
            $changed.Add($relative)
            Copy-Item -LiteralPath (Join-Path $payload $relative) -Destination $target -Force
            if ((Get-FileHash $target).Hash -ne $entries[$relative][0]) { throw "Deployment mismatch: $relative" }
        }
    }
    catch {
        foreach ($relative in $changed) {
            $target = Join-Path $GameRoot $relative
            $copy = Join-Path $backup $relative
            if (Test-Path $copy) { Copy-Item -LiteralPath $copy -Destination $target -Force }
            elseif (Test-Path $target) { Remove-Item -LiteralPath $target }
        }
        throw
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
