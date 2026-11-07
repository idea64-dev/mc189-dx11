@echo off
rem Regenerates tests\reference\JavaReference.inc and src\util\MathHelperTables.inc from the real JDK and MCP's MathHelper.java.
rem Run with Java 8 as "java" on PATH: the JVM Minecraft 1.8.9 shipped with.
rem Usage: generate.bat [path to MCP src\minecraft]   (default C:\MCP-919\src\minecraft)
setlocal
set MCP=%~1
if "%MCP%"=="" set MCP=C:\MCP-919\src\minecraft
set OUT=%TEMP%\mcdx11-reference
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"
javac --release 8 -encoding UTF-8 -d "%OUT%" "%~dp0stubs\net\minecraft\util\Vec3i.java" "%MCP%\net\minecraft\util\MathHelper.java" "%~dp0GenerateReference.java" || exit /b 1
java -cp "%OUT%" GenerateReference "%~dp0..\..\tests\reference\JavaReference.inc" "%~dp0..\..\src\util\MathHelperTables.inc" || exit /b 1
