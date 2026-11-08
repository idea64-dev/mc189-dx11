# Compiles the whole MCP 9.19 source (deobfuscated Minecraft 1.8.9) against the libraries of an installed launcher,
# so reference generators can call the real game code by its MCP names (block registry dumps, world generation...).
#
#   powershell -ExecutionPolicy Bypass -File tools\reference\compile_mcp.ps1
#
# Output: %TEMP%\mcdx11-mcp\classes (compiled game) and %TEMP%\mcdx11-mcp\classpath.txt (classes + libraries +
# the client jar for assets). Needs a JDK whose javac supports --release 8 and the MCP source in C:\MCP-919.
param(
    [string]$Mcp = "C:\MCP-919\src\minecraft",
    [string]$Launcher = "$env:APPDATA\.tlauncher\legacy\Minecraft\game",
    [string]$Out = (Join-Path $env:TEMP "mcdx11-mcp")
)
$ErrorActionPreference = "Stop"

$version = Get-Content (Join-Path $Launcher "versions\1.8.9\1.8.9.json") -Raw | ConvertFrom-Json
$libs = @()
foreach ($lib in $version.libraries) {
    if ($lib.natives) { continue }
    $parts = $lib.name.Split(":")
    $path = Join-Path $Launcher ("libraries\" + $parts[0].Replace(".", "\") + "\" + $parts[1] + "\" + $parts[2] + "\" + $parts[1] + "-" + $parts[2] + ".jar")
    if (Test-Path $path) { $libs += $path }
}

$classes = Join-Path $Out "classes"
if (Test-Path $classes) { Remove-Item -Recurse -Force $classes }
New-Item -ItemType Directory -Force $classes | Out-Null

# The decompiled source needs a few casts that javac 9+ no longer infers; compile a patched copy.
$src = Join-Path $Out "src"
if (Test-Path $src) { Remove-Item -Recurse -Force $src }
Copy-Item -Recurse $Mcp $src
$patches = @(
    @{ File = "net/minecraft/util/Cartesian.java"; From = "return (Iterator<T[]>)(this.iterables.length"; To = "return (Iterator<T[]>)(Iterator<?>)(this.iterables.length" }
)
foreach ($p in $patches) {
    $f = Join-Path $src $p.File
    $text = [IO.File]::ReadAllText($f)
    if (-not $text.Contains($p.From)) { throw "patch target not found in $($p.File)" }
    [IO.File]::WriteAllText($f, $text.Replace($p.From, $p.To))
}

$sources = Join-Path $Out "sources.txt"
Get-ChildItem $src -Recurse -Filter *.java | ForEach-Object { '"' + $_.FullName.Replace([char]92, [char]47) + '"' } | Set-Content $sources -Encoding ASCII

$ErrorActionPreference = "Continue"
& javac --release 8 -nowarn -encoding UTF-8 -proc:none -cp ($libs -join ";") -d $classes "@$sources" 2>&1 | ForEach-Object { "$_" } | Where-Object { $_ -match "error" } | Select-Object -First 30
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

(@($classes) + $libs + @(Join-Path $Launcher "versions\1.8.9\1.8.9.jar")) -join ";" | Set-Content (Join-Path $Out "classpath.txt") -Encoding ASCII
"compiled to $classes"
