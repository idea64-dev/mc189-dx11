# Stage 1 smoke test: start the game, drive the window, capture screenshots, close it, print the log.
param([string]$Preset = "debug")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$runDir = Join-Path $root "run"
New-Item -ItemType Directory -Force $runDir | Out-Null

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class W {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int hh, bool repaint);
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public int type; public InputUnion u; }
    [StructLayout(LayoutKind.Explicit)] public struct InputUnion { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public IntPtr extra; }
    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort vk, scan; public uint flags, time; public IntPtr extra; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
    // Physical-keyboard-like key press: scan code only, E0-extended when scan > 0xFF.
    public static void Key(int scan) {
        uint ext = scan > 0xFF ? 1u : 0u;
        var down = new INPUT { type = 1 }; down.u.ki.scan = (ushort)(scan & 0xFF); down.u.ki.flags = 8u | ext;
        var up = down; up.u.ki.flags = 8u | 2u | ext;
        SendInput(2, new[] { down, up }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void Mouse(uint flags, int dx, int dy) {
        var i = new INPUT { type = 0 }; i.u.mi.dx = dx; i.u.mi.dy = dy; i.u.mi.dwFlags = flags;
        SendInput(1, new[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, System.Text.StringBuilder s, int n);
}
"@
[W]::SetProcessDPIAware() | Out-Null

function Shot($hwnd, $name) {
    $r = New-Object W+RECT; [W]::GetClientRect($hwnd, [ref]$r) | Out-Null
    $p = New-Object W+POINT; [W]::ClientToScreen($hwnd, [ref]$p) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R)), ([Math]::Max(1, $r.B))
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($p.X, $p.Y, 0, 0, $bmp.Size)
    $center = $bmp.GetPixel([int]($bmp.Width / 2), [int]($bmp.Height / 2))
    $bmp.Save((Join-Path $runDir "$name.png")); $g.Dispose(); $bmp.Dispose()
    "{0}: client {1}x{2}, centre pixel #{3:X2}{4:X2}{5:X2}" -f $name, $r.R, $r.B, $center.R, $center.G, $center.B
}
function Title($hwnd) { $sb = New-Object System.Text.StringBuilder 256; [W]::GetWindowText($hwnd, $sb, 256) | Out-Null; $sb.ToString() }

$exe = Join-Path $root "build\$Preset\MinecraftDX11.exe"
$proc = Start-Process $exe -ArgumentList "--gameDir", $runDir, "--unknownFlag" -PassThru
Start-Sleep -Seconds 3
$proc.Refresh(); $hwnd = $proc.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { throw "no window" }

Shot $hwnd "smoke_1_start"
"title: " + (Title $hwnd)
# WM_GETICON: ICON_SMALL = 0, ICON_BIG = 1
"icons: small=" + ([W]::SendMessage($hwnd, 0x7F, [IntPtr]0, [IntPtr]0) -ne [IntPtr]::Zero) + " big=" + ([W]::SendMessage($hwnd, 0x7F, [IntPtr]1, [IntPtr]0) -ne [IntPtr]::Zero)

[W]::SetForegroundWindow($hwnd) | Out-Null; Start-Sleep -Milliseconds 300
# A, LShift+A, right Ctrl, numpad Enter, Up arrow (scan codes, E0 = extended)
foreach ($k in 0x1E, 0x2A, 0x1E, 0xE01D, 0xE01C, 0xE048) { [W]::Key($k); Start-Sleep -Milliseconds 30 }
# click centre to grab, move the mouse, scroll, release with Escape
$c = New-Object W+POINT; $cr = New-Object W+RECT; [W]::GetClientRect($hwnd, [ref]$cr) | Out-Null
$c.X = [int]($cr.R / 2); $c.Y = [int]($cr.B / 2); [W]::ClientToScreen($hwnd, [ref]$c) | Out-Null
[System.Windows.Forms.Cursor]::Position = New-Object System.Drawing.Point $c.X, $c.Y
[W]::Mouse(0x2, 0, 0); [W]::Mouse(0x4, 0, 0); Start-Sleep -Milliseconds 200
for ($i = 0; $i -lt 10; $i++) { [W]::Mouse(0x1, 15, -5); Start-Sleep -Milliseconds 20 }
Start-Sleep -Milliseconds 1200
[W]::Key(0x01); Start-Sleep -Milliseconds 200
[W]::Key(0x57)
Start-Sleep -Seconds 1
Shot $hwnd "smoke_2_fullscreen"
[W]::Key(0x57)
Start-Sleep -Seconds 1
Shot $hwnd "smoke_3_windowed"

[W]::MoveWindow($hwnd, 100, 100, 1000, 700, $true) | Out-Null
Start-Sleep -Seconds 1
Shot $hwnd "smoke_4_resized"
"title: " + (Title $hwnd)

$proc.CloseMainWindow() | Out-Null
if (-not $proc.WaitForExit(5000)) { $proc.Kill(); throw "did not exit on WM_CLOSE" }
"exit code: " + $proc.ExitCode
"--- latest.log ---"
Get-Content (Join-Path $runDir "logs\latest.log")
