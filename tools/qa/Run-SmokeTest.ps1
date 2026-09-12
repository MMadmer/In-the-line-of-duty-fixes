[CmdletBinding()]
param(
    [string]$GameRoot = 'D:\Games\S.T.A.L.K.E.R',
    [string]$Save = 'admin_quicksave',
    [int]$HoldSeconds = 5,
    [int]$LoadTimeoutSeconds = 240,
    [switch]$MenuOnly
)

# The gate the Lua unit tests cannot be: the game is started and told to load a save, and the log is read back.
# The engine wraps every script in its own namespace header before compiling, so a script that is one local over
# its budget loads in desktop Lua, passes luac and packaging, and then makes the game die on the main menu with
# nothing in the log but the first script that touched the namespace. Only a launch finds that.
#
# The engine refuses -fsltx here (it exits before it writes a log), so the run uses the player's own app-data
# root. user.ltx is restored byte for byte afterwards and the save tree is hashed on both sides.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$version = [regex]::Match((Get-Content "$repo\CMakeLists.txt" -Raw),
    'project\(InTheLineOfDutyFixes VERSION ([0-9]+\.[0-9]+\.[0-9]+)').Groups[1].Value
$appData = 'C:\Users\Public\Documents\STALKER-SHOC'
$run = Join-Path $repo "qa\smoke-$version"
New-Item -ItemType Directory -Force -Path $run | Out-Null

function Save-Hashes {
    Get-ChildItem (Join-Path $appData 'savedgames') -Recurse -File |
        Sort-Object FullName | ForEach-Object { "$($_.Name)`t$((Get-FileHash $_.FullName).Hash)" }
}

if (Get-Process -Name 'XR_3DA' -ErrorAction SilentlyContinue) { throw 'The game is already running.' }
$savesBefore = Save-Hashes
Copy-Item "$appData\user.ltx" "$run\user.ltx.before" -Force
$userBefore = (Get-FileHash "$appData\user.ltx").Hash

Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class Smoke {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  // Offscreen on the ordinary desktop: the engine keeps rendering, and nothing lands in front of the user.
  public static void Offscreen(uint pid) {
    EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p);
      if (p == pid && IsWindowVisible(h)) SetWindowPos(h, IntPtr.Zero, -4000, -4000, 0, 0, 0x0001 | 0x0004 | 0x0010);
      return true; }, IntPtr.Zero); }
}
'@

$arguments = if ($MenuOnly) { '' } else { "-start server($Save/single/alife/load) client(localhost)" }
$start = @{ FilePath = Join-Path $GameRoot 'bin\XR_3DA.exe'; WorkingDirectory = $GameRoot; PassThru = $true }
if ($arguments) { $start.ArgumentList = $arguments }
$process = Start-Process @start
$log = Join-Path $appData 'logs\xray_admin.log'
$deadline = (Get-Date).AddSeconds($LoadTimeoutSeconds)
$state = 'timeout'
while ((Get-Date) -lt $deadline) {
    [Smoke]::Offscreen([uint32]$process.Id)
    if ($process.HasExited) { $state = 'exited'; break }
    if (Test-Path $log) {
        $text = Get-Content $log -Raw -ErrorAction SilentlyContinue
        if ($text -match 'FATAL ERROR') { $state = 'fatal'; break }
        if ($MenuOnly) { if ($text -match 'Starting engine') { $state = 'menu'; break } }
        elseif ($text -match '\* phase time:.*\bload\b|Loading objects\.\.\.|\* Loading level') { $state = 'loaded'; break }
    }
    Start-Sleep -Milliseconds 500
}
if ($state -eq 'loaded' -or $state -eq 'menu') { Start-Sleep -Seconds $HoldSeconds }
[Smoke]::Offscreen([uint32]$process.Id)
if (-not $process.HasExited) { $process.CloseMainWindow() | Out-Null; Start-Sleep -Seconds 6 }
if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force; Start-Sleep -Seconds 2 }
Start-Sleep -Seconds 1

Copy-Item $log "$run\xray-$state.log" -Force -ErrorAction SilentlyContinue
$savesAfter = Save-Hashes
# The engine rewrites user.ltx on a clean exit; the player's copy is put back exactly as it was.
if ((Get-FileHash "$appData\user.ltx").Hash -ne $userBefore) { Copy-Item "$run\user.ltx.before" "$appData\user.ltx" -Force }
$restored = (Get-FileHash "$appData\user.ltx").Hash -eq $userBefore

"version=$version"
"state=$state"
"savesUnchanged=$((($savesBefore -join "`n") -eq ($savesAfter -join "`n")))"
"userLtxRestored=$restored"
"log=$run\xray-$state.log"
if (Test-Path "$run\xray-$state.log") {
    $bad = Select-String -LiteralPath "$run\xray-$state.log" -Pattern '^!|LUA error|FATAL ERROR|Assertion|Access violation'
    "problemLines=$($bad.Count)"
    $bad | Select-Object -First 20 | ForEach-Object { "  $($_.Line)" }
}
Get-Process -Name 'XR_3DA','BugTrap','InTheLineOfDutyFixesUpdater' -ErrorAction SilentlyContinue |
    ForEach-Object { "still running: $($_.Name) $($_.Id)" }
