::
: \file build.bat
: \author Isaiah Lateer
: 
: Used to build the project.
: 

@echo off
setlocal enabledelayedexpansion

for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy ^
    Bypass -Command "$vs = Get-CimInstance -Namespace root/cimv2/vs " ^
    "MSFT_VSInstance | Sort-Object InstallationVersion -Descending | " ^
    "Select-Object -First 1; if ($vs) { $vs.InstallLocation } else { exit 1 }"`
    ) do (
    set "devcmd=%%i"
)

set "devcmd=%devcmd%\Common7\Tools\VsDevCmd.bat"
if not exist "%devcmd%" (
    echo [ERROR] Failed to find developer command prompt.
    exit /b 1
)

call "%devcmd%" > nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize build environment.
    exit /b 1
)

msbuild opengl_context.vcxproj /p:Configuration=Release /p:Platform=x64
