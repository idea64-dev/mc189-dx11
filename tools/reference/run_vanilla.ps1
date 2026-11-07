# Starts the vanilla Minecraft 1.8.9 client (Java 8, offline) from an installed launcher folder, waits, presses
# F2 and copies the screenshot, so the port's frames can be compared with the original pixel for pixel.
#
#   powershell -ExecutionPolicy Bypass -File tools\reference\run_vanilla.ps1 -Output run\vanilla_menu.png
#
# Uses its own game directory (run_vanilla\) so the real installation's options and saves are untouched.
param(
    [string]$Launcher = "$env:APPDATA\.tlauncher\legacy\Minecraft\game",
    [string]$GameDir = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "run_vanilla"),
    [string]$Output = "",
    [int]$WaitSeconds = 12,
    [int]$Width = 854,
    [int]$Height = 480
)
$ErrorActionPreference = "Stop"

$version = Get-Content (Join-Path $Launcher "versions\1.8.9\1.8.9.json") -Raw | ConvertFrom-Json
$classpath = @()
foreach ($lib in $version.libraries) {
    if ($lib.natives) { continue }
    $allowed = $true
    if ($lib.rules) {
        $allowed = $false
        foreach ($rule in $lib.rules) {
            $matches = (-not $rule.os) -or ($rule.os.name -eq "windows")
            if ($matches) { $allowed = ($rule.action -eq "allow") }
        }
    }
    if (-not $allowed) { continue }
    $parts = $lib.name.Split(":")
    $path = Join-Path $Launcher ("libraries\" + $parts[0].Replace(".", "\") + "\" + $parts[1] + "\" + $parts[2] + "\" + $parts[1] + "-" + $parts[2] + ".jar")
    if (Test-Path $path) { $classpath += $path }
}
$classpath += (Join-Path $Launcher "versions\1.8.9\1.8.9.jar")

New-Item -ItemType Directory -Force $GameDir | Out-Null
$shots = Join-Path $GameDir "screenshots"
$before = @(Get-ChildItem $shots -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object FullName)

$javaArgs = @(
    "-Xmx1G", "-Djava.library.path=`"$(Join-Path $Launcher 'versions\1.8.9\natives')`"",
    "-cp", "`"$($classpath -join ';')`"", "net.minecraft.client.main.Main",
    "--username", "Player", "--version", "1.8.9", "--gameDir", "`"$GameDir`"",
    "--assetsDir", "`"$(Join-Path $Launcher 'assets')`"", "--assetIndex", "1.8",
    "--uuid", "00000000000000000000000000000000", "--accessToken", "0", "--userProperties", "{}", "--userType", "legacy",
    "--width", "$Width", "--height", "$Height"
)
# log4j writes logs/ relative to the working directory
$proc = Start-Process java -ArgumentList $javaArgs -WorkingDirectory $GameDir -PassThru
try {
    Start-Sleep -Seconds $WaitSeconds
    $proc.Refresh()
    if ($proc.HasExited) { throw "vanilla client exited with code $($proc.ExitCode)" }

    Add-Type @"
using System; using System.Runtime.InteropServices;
public static class V {
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public int type; public KI ki; public long pad; }
    [StructLayout(LayoutKind.Sequential)] public struct KI { public ushort vk, scan; public uint flags, time; public IntPtr extra; }
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] i, int size);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    public static void Key(ushort scan) {
        var d = new INPUT { type = 1 }; d.ki.scan = scan; d.ki.flags = 8;
        var u = d; u.ki.flags = 10;
        SendInput(2, new[] { d, u }, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@
    [V]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 300
    [V]::Key(0x3C) # F2
    $file = $null
    for ($i = 0; $i -lt 50 -and -not $file; $i++) {
        Start-Sleep -Milliseconds 100
        $file = Get-ChildItem $shots -Filter *.png -ErrorAction SilentlyContinue | Where-Object { $before -notcontains $_.FullName } | Select-Object -First 1
    }
    if (-not $file) { throw "no screenshot was written" }
    # ImageIO writes the PNG in pieces; wait until the size settles.
    $size = -1
    for ($i = 0; $i -lt 50; $i++) {
        Start-Sleep -Milliseconds 200
        $now = (Get-Item $file.FullName).Length
        if ($now -eq $size -and $now -gt 0) { break }
        $size = $now
    }
    if ($Output) { Copy-Item $file.FullName $Output -Force; "saved $Output" } else { "saved $($file.FullName)" }
} finally {
    if (-not $proc.HasExited) { $proc.CloseMainWindow() | Out-Null; if (-not $proc.WaitForExit(8000)) { $proc.Kill() } }
}
