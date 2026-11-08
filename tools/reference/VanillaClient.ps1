# Builds the java command line for the vanilla Minecraft 1.8.9 client (offline, Java 8) from an installed
# launcher folder. Dot-sourced by run_vanilla.ps1 and ui_scenario.ps1.

function Get-VanillaJavaArgs([string]$Launcher, [string]$GameDir, [int]$Width, [int]$Height) {
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

    @(
        "-Xmx2G", "-Djava.library.path=`"$(Join-Path $Launcher 'versions\1.8.9\natives')`"",
        "-cp", "`"$($classpath -join ';')`"", "net.minecraft.client.main.Main",
        "--username", "Player", "--version", "1.8.9", "--gameDir", "`"$GameDir`"",
        "--assetsDir", "`"$(Join-Path $Launcher 'assets')`"", "--assetIndex", "1.8",
        "--uuid", "00000000000000000000000000000000", "--accessToken", "0", "--userProperties", "{}", "--userType", "legacy",
        "--width", "$Width", "--height", "$Height"
    )
}
