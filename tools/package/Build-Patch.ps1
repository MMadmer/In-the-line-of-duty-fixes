[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BaseVersion
)

# Cuts the update patch for the current project version against an already packaged previous candidate.
# It carries only what changed, and a manifest that still describes the complete installed file set, so a
# patch install and a full install end up with the same files.
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$cmake = Get-Content -LiteralPath (Join-Path $repository 'CMakeLists.txt') -Raw
$versionMatch = [regex]::Match($cmake, 'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)')
if (-not $versionMatch.Success) { throw 'Cannot read the product version.' }
$version = $versionMatch.Groups[1].Value
if ($version -eq $BaseVersion) { throw "The patch base must be an earlier version than $version." }

$artifacts = Join-Path $repository 'artifacts\candidate'
$current = Join-Path $artifacts "payload-$version"
$previous = Join-Path $artifacts "payload-$BaseVersion"
foreach ($tree in @($current, $previous)) {
    if (-not (Test-Path -LiteralPath $tree)) { throw "Missing packaged candidate: $tree" }
}
$archive = Join-Path $artifacts "In-the-line-of-duty-fixes-$version-Update_Patch.zip"
if (Test-Path -LiteralPath $archive) { throw "Candidate archive already exists: $archive" }

$manifestPath = '.ild-fixes/update-manifest.txt'
$rows = Get-Content -LiteralPath (Join-Path $current '.ild-fixes\update-manifest.txt')
if ($rows[0] -ne 'schema=ild-fixes.update/1' -or $rows[1] -ne "version=$version") {
    throw 'The packaged manifest is not the full-archive form of this version.'
}
$files = $rows | Select-Object -Skip 2

function Get-Relative([string]$Root, [IO.FileInfo]$File) {
    $Root = (Resolve-Path -LiteralPath $Root).Path.TrimEnd('\')
    $File.FullName.Substring($Root.Length + 1).Replace('\', '/')
}

# The manifest is written fresh below, so the packaged copy is never carried over as payload.
$changed = foreach ($file in Get-ChildItem -LiteralPath $current -Recurse -File) {
    $relative = Get-Relative $current $file
    if ($relative -eq $manifestPath) { continue }
    $before = Join-Path $previous ($relative -replace '/', '\')
    if ((Test-Path -LiteralPath $before) -and
        (Get-FileHash -LiteralPath $before).Hash -eq (Get-FileHash -LiteralPath $file.FullName).Hash) { continue }
    $relative
}
if (-not $changed) { throw "Nothing changed since $BaseVersion; a patch would be empty." }

$declared = $files | ForEach-Object { ($_ -split "`t")[2] }
foreach ($relative in $changed) {
    if ($declared -notcontains $relative) { throw "Changed file is not declared in the manifest: $relative" }
}

$staging = Join-Path $artifacts "patch-$version"
if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
New-Item -ItemType Directory -Force -Path $staging | Out-Null
foreach ($relative in $changed) {
    $destination = Join-Path $staging ($relative -replace '/', '\')
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath (Join-Path $current ($relative -replace '/', '\')) -Destination $destination
}
$manifest = @("schema=ild-fixes.update/2", "version=$version", 'kind=patch', "base=$BaseVersion") + $files
$manifestFile = Join-Path $staging ($manifestPath -replace '/', '\')
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $manifestFile) | Out-Null
[IO.File]::WriteAllText($manifestFile, ($manifest -join "`n") + "`n", (New-Object Text.UTF8Encoding $false))

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$stream = [IO.Compression.ZipFile]::Open($archive, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($relative in (@($manifestPath) + $changed | Sort-Object)) {
        $source = Join-Path $staging ($relative -replace '/', '\')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $stream, $source, $relative, [IO.Compression.CompressionLevel]::Optimal)
    }
}
finally { $stream.Dispose() }
Remove-Item -LiteralPath $staging -Recurse -Force

# Read the archive back and check it against the manifest it carries.
$stream = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $names = $stream.Entries | ForEach-Object { $_.FullName }
    foreach ($name in $names) {
        if ($name -ne $manifestPath -and $declared -notcontains $name) { throw "Undeclared archive entry: $name" }
    }
    foreach ($row in $files) {
        $parts = $row -split "`t"
        $entry = $stream.Entries | Where-Object { $_.FullName -eq $parts[2] }
        if (-not $entry) { continue }
        if ($entry.Length -ne [long]$parts[1]) { throw "Size mismatch in the archive: $($parts[2])" }
    }
}
finally { $stream.Dispose() }

"Archive=$archive"
"SHA256=$((Get-FileHash -LiteralPath $archive).Hash)"
"Base=$BaseVersion"
"PatchFiles=$(($changed | Measure-Object).Count + 1)"
"Candidate only; release/runtime QA is not implied by packaging."
