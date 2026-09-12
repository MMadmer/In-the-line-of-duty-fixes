[CmdletBinding()]
param(
    [string]$GameRoot = 'D:\Games\S.T.A.L.K.E.R'
)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$cmake = Get-Content -LiteralPath (Join-Path $repository 'CMakeLists.txt') -Raw
$versionMatch = [regex]::Match($cmake, 'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)')
if (-not $versionMatch.Success) { throw 'Cannot read the product version.' }
$version = $versionMatch.Groups[1].Value
$artifacts = Join-Path $repository 'artifacts\candidate'
$packageRoot = Join-Path $artifacts "payload-$version"
if (Test-Path -LiteralPath $packageRoot) { throw "Candidate output already exists: $packageRoot" }
New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null

$sourceMap = [ordered]@{
    'bin/dinput8.dll' = 'build/Release/dinput8.dll'
    'InTheLineOfDutyFixesUpdater.exe' = 'build/updater/InTheLineOfDutyFixesUpdater.exe'
    'gamedata/scripts/ild_fix_ui.script' = 'payload/gamedata/scripts/ild_fix_ui.script'
    'gamedata/scripts/ild_gameplay.script' = 'payload/gamedata/scripts/ild_gameplay.script'
    'gamedata/scripts/ild_script_repairs.script' = 'payload/gamedata/scripts/ild_script_repairs.script'
    'gamedata/scripts/ild_recipe_repairs.script' = 'payload/gamedata/scripts/ild_recipe_repairs.script'
    'gamedata/config/ui/ild_fixes_update.xml' = 'payload/gamedata/config/ui/ild_fixes_update.xml'
    'gamedata/config/ui/ild_fixes_options.xml' = 'payload/gamedata/config/ui/ild_fixes_options.xml'
    'gamedata/config/text/rus/ild_fixes_text.xml' = 'payload/gamedata/config/text/rus/ild_fixes_text.xml'
    'README-InTheLineOfDutyFixes.txt' = 'packaging/README-InTheLineOfDutyFixes.txt'
}
# The manifest lives inside the owned .ild-fixes directory, so a manual install leaves nothing unmanaged.
$manifestPath = '.ild-fixes/update-manifest.txt'
$paths = @($sourceMap.Keys) + @('.ild-fixes/version.txt', '.ild-fixes/managed-files.txt', $manifestPath)

# A stale helper binary would offer its own version forever; its embedded product version must match CMake.
$updaterVersion = (Get-Item -LiteralPath (Join-Path $repository $sourceMap['InTheLineOfDutyFixesUpdater.exe'])).VersionInfo.ProductVersion
if ($updaterVersion -ne $version) { throw "Updater product version '$updaterVersion' does not match project version $version." }

$owned = @()
$installedManaged = Join-Path $GameRoot '.ild-fixes\managed-files.txt'
if (Test-Path -LiteralPath $installedManaged) { $owned = @(Get-Content -LiteralPath $installedManaged) }
foreach ($relative in $paths) {
    $destination = Join-Path $GameRoot $relative.Replace('/', '\')
    if ((Test-Path -LiteralPath $destination) -and $relative -notin $owned) {
        throw "Foreign destination collision: $destination"
    }
}

foreach ($relative in $sourceMap.Keys) {
    $source = Join-Path $repository $sourceMap[$relative]
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing runtime file: $source" }
    $destination = Join-Path $packageRoot $relative.Replace('/', '\')
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
        throw "Staging hash mismatch: $relative"
    }
}
$control = Join-Path $packageRoot '.ild-fixes'
New-Item -ItemType Directory -Force -Path $control | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $control 'version.txt'), "$version`n", $utf8)
[IO.File]::WriteAllText((Join-Path $control 'managed-files.txt'), (($paths | Sort-Object) -join "`n") + "`n", $utf8)
$manifest = [Collections.Generic.List[string]]::new()
$manifest.Add('schema=ild-fixes.update/1')
$manifest.Add("version=$version")
foreach ($relative in ($paths | Sort-Object)) {
    if ($relative -eq $manifestPath) { continue }
    $file = Get-Item -LiteralPath (Join-Path $packageRoot $relative.Replace('/', '\'))
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest.Add("$hash`t$($file.Length)`t$relative")
}
[IO.File]::WriteAllText((Join-Path $control 'update-manifest.txt'), ($manifest -join "`n") + "`n", $utf8)
$archive = Join-Path $artifacts "In-the-line-of-duty-fixes-$version-Setup_Manual.zip"
if (Test-Path -LiteralPath $archive) { throw "Candidate archive already exists: $archive" }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($packageRoot, $archive, [IO.Compression.CompressionLevel]::Optimal, $false)
$archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
Write-Output "Candidate only; release/runtime QA is not implied by packaging."
Write-Output "Archive=$archive"
Write-Output "SHA256=$archiveHash"
Write-Output "PayloadFiles=$($paths.Count)"
