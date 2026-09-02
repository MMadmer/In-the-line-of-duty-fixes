param(
    [int]$Port = 8792,
    [ValidateSet('Full', 'Patch', 'Major', 'Both', 'BadDigest', 'Empty')]
    [string]$Scenario = 'Full',
    [string]$InstalledVersion = '0.9.0',
    [string]$AssetRoot = (Join-Path $PSScriptRoot '..\..\qa\mock-assets')
)

$ErrorActionPreference = 'Stop'
$AssetRoot = [IO.Path]::GetFullPath($AssetRoot)
New-Item -ItemType Directory -Force -Path $AssetRoot | Out-Null
$prefix = 'In-the-line-of-duty-fixes-'
$installed = [version]$InstalledVersion
$nextVersion = "$($installed.Major).$($installed.Minor).$($installed.Build + 1)"
$versions = @($InstalledVersion, $nextVersion)
if ($Scenario -in 'Major', 'Both') { $versions += "$($installed.Major + 1).0.0" }
if ($Scenario -eq 'Empty') { $versions = @() }
$assets = @{}
$releases = foreach ($version in $versions) {
    $releaseAssets = foreach ($suffix in @('Setup_Manual') + $(if ($Scenario -in 'Patch', 'Both') { 'Update_Patch' })) {
        if (-not $suffix) { continue }
        $name = "$prefix$version-$suffix.zip"
        $path = Join-Path $AssetRoot $name
        if (-not (Test-Path -LiteralPath $path)) {
            $bytes = [byte[]]::new(128 * 1024)
            for ($i = 0; $i -lt $bytes.Length; ++$i) { $bytes[$i] = [byte](($i * 31 + $name.Length) % 251) }
            [IO.File]::WriteAllBytes($path, $bytes)
        }
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($Scenario -eq 'BadDigest') { $hash = '0' * 64 }
        $assets[$name] = $path
        @{ name = $name; size = (Get-Item -LiteralPath $path).Length; digest = "sha256:$hash"
           browser_download_url = "http://127.0.0.1:$Port/assets/$name" }
    }
    @{ tag_name = $version; draft = $false; prerelease = $false
       html_url = "https://github.com/MMadmer/In-the-line-of-duty-fixes/releases/tag/$version"
       body = "## RU`n`n## Тема`n`nКонсоль без лишнего шума`n`n## Изменения`n`n* Исправлен поток ложных ошибок в консоли.`n* Строка ввода остаётся на месте.`n`n## EN`n`n## Theme`n`nA quieter console`n`n## Changes`n`n* Fixed recurring false console errors.`n* Input is preserved.`n"
       assets = @($releaseAssets) }
}
$payload = ConvertTo-Json -InputObject @($releases) -Depth 8 -Compress
$listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, $Port)
$listener.Start()
Write-Output "http://127.0.0.1:$Port/releases"
try {
    while ($true) {
        $client = $listener.AcceptTcpClient()
        try {
            $stream = $client.GetStream()
            $buffer = [byte[]]::new(8192)
            $read = $stream.Read($buffer, 0, $buffer.Length)
            if ($read -le 0) { continue }
            $request = [Text.Encoding]::ASCII.GetString($buffer, 0, $read)
            $target = ($request -split "`r`n")[0].Split(' ')[1]
            if ($target.StartsWith('/assets/')) {
                $name = [Uri]::UnescapeDataString([IO.Path]::GetFileName($target))
                if (-not $assets.ContainsKey($name)) { throw "Unknown mock asset: $name" }
                $body = [IO.File]::ReadAllBytes($assets[$name])
                $type = 'application/zip'
            } else {
                $body = [Text.Encoding]::UTF8.GetBytes($payload)
                $type = 'application/json'
            }
            $head = [Text.Encoding]::ASCII.GetBytes("HTTP/1.1 200 OK`r`nContent-Type: $type`r`nContent-Length: $($body.Length)`r`nConnection: close`r`n`r`n")
            $stream.Write($head, 0, $head.Length)
            $stream.Write($body, 0, $body.Length)
            $stream.Flush()
        } finally { $client.Close() }
    }
} finally { $listener.Stop() }
