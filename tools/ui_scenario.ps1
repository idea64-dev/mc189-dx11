# Drives the port or the vanilla 1.8.9 client through a scripted UI scenario and saves F2 screenshots, so
# screens can be compared frame by frame (tools/compare_screens.py --full).
#
#   powershell -ExecutionPolicy Bypass -File tools\ui_scenario.ps1 -Target port    -Scenario tools\scenarios\options.txt
#   powershell -ExecutionPolicy Bypass -File tools\ui_scenario.ps1 -Target vanilla -Scenario tools\scenarios\options.txt
#
# Each target runs in a fresh game directory run\cmp\<target>\game (options.txt is deleted, so both start
# from defaults) and writes run\cmp\<target>\<shot>.png.
#
# Scenario steps, one per line ('#' starts a comment). Coordinates are GUI (scaled) pixels; the click lands
# in the middle of the scaled pixel.
#   click X Y          left click
#   rclick X Y         right click
#   move X Y           move the cursor
#   drag X1 Y1 X2 Y2   press at 1, move to 2, release
#   wheel N            mouse wheel, N notches (positive = away from the user)
#   key CODE           LWJGL key code (DirectInput scan code; codes >= 0x80 are sent E0-extended)
#   hold CODE MS       hold a key down for MS milliseconds
#   look DX DY         relative mouse move in raw counts (turns the camera in game)
#   wait MS            sleep
#   shot NAME          press F2 and save the screenshot as NAME.png
#   scale N            GUI scale used for the coordinates from here on (default: auto, like ScaledResolution)
param(
    [ValidateSet("port", "vanilla")][string]$Target = "port",
    [Parameter(Mandatory = $true)][string]$Scenario,
    [string]$Preset = "debug",
    [string]$Launcher = "$env:APPDATA\.tlauncher\legacy\Minecraft\game",
    [int]$StartupSeconds = 0,
    [int]$StepDelayMs = 500,
    # options.txt lines to start with (e.g. "enableVsync:false", "maxFps:260"); defaults otherwise
    [string[]]$Option = @(),
    # keep the worlds of earlier runs (scenarios that load a world instead of creating one)
    [switch]$KeepSaves
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root "run\cmp\$Target"
$gameDir = Join-Path $outDir "game"
New-Item -ItemType Directory -Force $gameDir | Out-Null
Remove-Item (Join-Path $gameDir "options.txt") -ErrorAction SilentlyContinue
if (-not $KeepSaves) { Remove-Item (Join-Path $gameDir "saves") -Recurse -Force -ErrorAction SilentlyContinue }
# Comma-separated too: an array passed through -File arrives as one string.
$Option = @($Option | ForEach-Object { $_ -split "," } | Where-Object { $_ })
if ($Option.Count -gt 0) { Set-Content (Join-Path $gameDir "options.txt") $Option -Encoding ascii }

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class U {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public int type; public InputUnion u; }
    [StructLayout(LayoutKind.Explicit)] public struct InputUnion { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort vk, scan; public uint flags, time; public IntPtr extra; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
    public static void Key(int code) {
        uint ext = code >= 0x80 ? 1u : 0u;
        var down = new INPUT { type = 1 }; down.u.ki.scan = (ushort)(ext == 1 ? code - 0x80 : code); down.u.ki.flags = 8u | ext;
        var up = down; up.u.ki.flags = 8u | 2u | ext;
        SendInput(2, new[] { down, up }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void KeyState(int code, bool down) {
        uint ext = code >= 0x80 ? 1u : 0u;
        var i = new INPUT { type = 1 }; i.u.ki.scan = (ushort)(ext == 1 ? code - 0x80 : code); i.u.ki.flags = 8u | ext | (down ? 0u : 2u);
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void MouseMove(int dx, int dy) {
        var i = new INPUT { type = 0 }; i.u.mi.dwFlags = 1u; i.u.mi.dx = dx; i.u.mi.dy = dy;
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void Mouse(uint flags, uint data) {
        var i = new INPUT { type = 0 }; i.u.mi.dwFlags = flags; i.u.mi.mouseData = data;
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@
[U]::SetProcessDPIAware() | Out-Null

if ($Target -eq "port") {
    $exe = Join-Path $root "build\$Preset\MinecraftDX11.exe"
    $proc = Start-Process $exe -ArgumentList "--gameDir", "`"$gameDir`"" -PassThru
    if ($StartupSeconds -le 0) { $StartupSeconds = 4 }
} else {
    . (Join-Path $PSScriptRoot "reference\VanillaClient.ps1")
    $javaArgs = Get-VanillaJavaArgs $Launcher $gameDir 854 480
    $proc = Start-Process java -ArgumentList $javaArgs -WorkingDirectory $gameDir -PassThru
    if ($StartupSeconds -le 0) { $StartupSeconds = 14 }
}

function Get-Window {
    $proc.Refresh()
    if ($proc.HasExited) { throw "$Target exited with code $($proc.ExitCode)" }
    $proc.MainWindowHandle
}

function Get-AutoScale($hwnd) {
    $r = New-Object U+RECT; [U]::GetClientRect($hwnd, [ref]$r) | Out-Null
    $scale = 1
    while ($scale -lt 1000 -and [int][Math]::Floor($r.R / ($scale + 1)) -ge 320 -and [int][Math]::Floor($r.B / ($scale + 1)) -ge 240) { $scale++ }
    $scale
}

function Move-To($hwnd, $scale, [int]$x, [int]$y) {
    $p = New-Object U+POINT
    $p.X = $x * $scale + [int]($scale / 2); $p.Y = $y * $scale + [int]($scale / 2)
    [U]::ClientToScreen($hwnd, [ref]$p) | Out-Null
    [U]::SetCursorPos($p.X, $p.Y) | Out-Null
    # Let a frame render with the new position first: lists remember the mouse from their last draw.
    Start-Sleep -Milliseconds 250
}

function Save-Shot($hwnd, $name) {
    $dir = Join-Path $gameDir "screenshots"
    $before = @(Get-ChildItem $dir -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object FullName)
    [U]::Key(0x3C)
    $file = $null
    for ($i = 0; $i -lt 200 -and -not $file; $i++) {
        Start-Sleep -Milliseconds 100
        $file = Get-ChildItem $dir -Filter *.png -ErrorAction SilentlyContinue | Where-Object { $before -notcontains $_.FullName } | Select-Object -First 1
    }
    if (-not $file) { throw "${name}: no screenshot was written" }
    # ImageIO writes the PNG in pieces; wait until the size settles.
    $size = -1
    for ($i = 0; $i -lt 50; $i++) {
        Start-Sleep -Milliseconds 150
        $now = (Get-Item $file.FullName).Length
        if ($now -eq $size -and $now -gt 0) { break }
        $size = $now
    }
    $target = Join-Path $outDir "$name.png"
    Move-Item $file.FullName $target -Force
    "shot $target"
}

try {
    Start-Sleep -Seconds $StartupSeconds
    $hwnd = Get-Window
    if ($hwnd -eq [IntPtr]::Zero) { throw "no window" }
    [U]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 300
    $scale = 0

    foreach ($raw in Get-Content $Scenario) {
        $line = ($raw -replace "#.*$", "").Trim()
        if (-not $line) { continue }
        $a = $line -split "\s+"
        $hwnd = Get-Window
        $s = if ($scale -gt 0) { $scale } else { Get-AutoScale $hwnd }
        [U]::SetForegroundWindow($hwnd) | Out-Null
        switch ($a[0]) {
            "click"  { Move-To $hwnd $s $a[1] $a[2]; [U]::Mouse(0x2, 0); Start-Sleep -Milliseconds 60; [U]::Mouse(0x4, 0); Start-Sleep -Milliseconds $StepDelayMs }
            "rclick" { Move-To $hwnd $s $a[1] $a[2]; [U]::Mouse(0x8, 0); Start-Sleep -Milliseconds 60; [U]::Mouse(0x10, 0); Start-Sleep -Milliseconds $StepDelayMs }
            "move"   { Move-To $hwnd $s $a[1] $a[2]; Start-Sleep -Milliseconds $StepDelayMs }
            "drag"   {
                Move-To $hwnd $s $a[1] $a[2]; [U]::Mouse(0x2, 0); Start-Sleep -Milliseconds 150
                Move-To $hwnd $s $a[3] $a[4]; Start-Sleep -Milliseconds 150
                [U]::Mouse(0x4, 0); Start-Sleep -Milliseconds $StepDelayMs
            }
            "wheel"  { [U]::Mouse(0x800, [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$a[1] * 120), 0)); Start-Sleep -Milliseconds $StepDelayMs }
            "key"    { [U]::Key([Convert]::ToInt32($a[1], 10)); Start-Sleep -Milliseconds $StepDelayMs }
            "hold"   { [U]::KeyState([Convert]::ToInt32($a[1], 10), $true); Start-Sleep -Milliseconds ([int]$a[2]); [U]::KeyState([Convert]::ToInt32($a[1], 10), $false); Start-Sleep -Milliseconds $StepDelayMs }
            "look"   { [U]::MouseMove([int]$a[1], [int]$a[2]); Start-Sleep -Milliseconds $StepDelayMs }
            "wait"   { Start-Sleep -Milliseconds ([int]$a[1]) }
            "shot"   { Save-Shot $hwnd $a[1] }
            "scale"  { $scale = [int]$a[1] }
            default  { throw "unknown step: $line" }
        }
    }
} finally {
    if (-not $proc.HasExited) { $proc.CloseMainWindow() | Out-Null; if (-not $proc.WaitForExit(8000)) { $proc.Kill() } }
}
