@echo off
rem Usage: build.bat [debug|release]   (default: debug)
rem Needs the MSVC compiler and Windows SDK (Visual Studio or "Build Tools for Visual Studio"), CMake and Ninja.
rem If cl.exe is not already on PATH, the newest MSVC install is located with vswhere.
setlocal
set PRESET=%1
if "%PRESET%"=="" set PRESET=debug

where cl >nul 2>nul
if not errorlevel 1 goto build

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto nomsvc
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -all -prerelease -latest -products * -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT goto nomsvc
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" goto nomsvc

rem vcvars64.bat calls vswhere itself and expects it on PATH.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
rem CMake and Ninja bundled with Visual Studio, used when none are on PATH.
set "PATH=%PATH%;%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

:build
cd /d "%~dp0"
cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1
exit /b 0

:nomsvc
echo MSVC not found: install Visual Studio or Build Tools for Visual Studio with the C++ workload.
exit /b 1
