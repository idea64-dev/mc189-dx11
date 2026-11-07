@echo off
rem Usage: run.bat [debug|release] [game arguments...]
rem Starts the game with run\ as the game directory (logs, options.txt, saves).
setlocal
set PRESET=%1
if "%PRESET%"=="" set PRESET=debug
cd /d "%~dp0"
if not exist run mkdir run
shift
set ARGS=
:collect
if "%~1"=="" goto launch
set ARGS=%ARGS% %1
shift
goto collect
:launch
"build\%PRESET%\MinecraftDX11.exe" --gameDir run %ARGS%
