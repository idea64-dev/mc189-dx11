# Regenerates the reference data in tests\reference\ that comes from the game code itself (NBT, registries...).
# Needs the MCP source compiled by compile_mcp.ps1 and Java 8 as "java" on PATH (javac may be newer).
#
#   powershell -ExecutionPolicy Bypass -File tools\reference\generate_game.ps1
param([string]$Out = (Join-Path $env:TEMP "mcdx11-mcp"))
$ErrorActionPreference = "Stop"
$cpFile = Join-Path $Out "classpath.txt"
if (-not (Test-Path $cpFile)) { throw "Run tools\reference\compile_mcp.ps1 first." }
$gen = Join-Path $Out "gen"
New-Item -ItemType Directory -Force $gen | Out-Null
# The classpath is too long for a cmd line variable; Java 8 reads CLASSPATH from the environment.
$env:CLASSPATH = "$gen;" + (Get-Content $cpFile -Raw).Trim()
& javac --release 8 -nowarn -encoding UTF-8 -d $gen (Join-Path $PSScriptRoot "GenerateGameReference.java")
if ($LASTEXITCODE -ne 0) { throw "javac failed" }
& java GenerateGameReference (Join-Path $PSScriptRoot "..\..\tests\reference")
if ($LASTEXITCODE -ne 0) { throw "generator failed" }
