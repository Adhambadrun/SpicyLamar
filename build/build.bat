@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title SpicyLamar - C++ Build

:: Always operate from the repository root, no matter where this script was launched from.
:: This file lives in build\, so the repository root is one directory up.
pushd "%~dp0.." >nul
cd /d "%~dp0.."

echo ==========================================================
echo  SPICY LAMAR v4.2 - C++ PORTABLE BUILD
echo ==========================================================

:: ---------------------------------------------------------------------------
:: 1. Locate Visual Studio (self-bootstrapping, no Native Tools prompt needed)
:: ---------------------------------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)
if not defined VSPATH (
    echo [ERROR] Visual Studio 2022 / Build Tools with the C++ workload not found.
    echo         Install it from https://visualstudio.microsoft.com/downloads/
    echo         or use:  powershell -ExecutionPolicy Bypass -File build\build.ps1
    pause
    exit /b 1
)
call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to initialize the MSVC environment (!VSPATH!\VC\Auxiliary\Build\vcvars64.bat)
    pause
    exit /b 1
)
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cl.exe not found after vcvars - is the "Desktop development with C++" workload installed?
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: 2. Prepare output directories
:: ---------------------------------------------------------------------------
if not exist "build\obj" mkdir "build\obj"
if not exist "dist"      mkdir "dist"

:: ---------------------------------------------------------------------------
:: 3. Compile resources. rc.exe resolves "icon.ico" relative to app.rc.
:: ---------------------------------------------------------------------------
echo [1/3] Compiling resources...
pushd "resources" >nul
rc.exe /nologo /fo "..\build\obj\app.res" app.rc
if errorlevel 1 (
    echo [ERROR] rc.exe failed - see message above.
    popd
    pause
    exit /b 1
)
popd >nul

:: ---------------------------------------------------------------------------
:: 4. Compile the monolith
:: ---------------------------------------------------------------------------
echo [2/3] Compiling main.cpp...
cl.exe /nologo /std:c++20 /O2 /Oi /GL /Gy /MT /utf-8 ^
    /DUNICODE /D_UNICODE /DSPICY_LAMAR_QUANTUM /DSPICY_LAMAR_TURBO /DNDEBUG /EHsc ^
    /c "src\main.cpp" /Fo:"build\obj\main.obj"
if errorlevel 1 (
    echo [ERROR] cl.exe compilation failed.
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: 5. Link the portable single-file executable
:: ---------------------------------------------------------------------------
echo [3/3] Linking "SpicyLamar.exe"...
link.exe /nologo /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,10.0 /MACHINE:X64 ^
    "build\obj\main.obj" "build\obj\app.res" ^
    comctl32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib uxtheme.lib ^
    winmm.lib avrt.lib dwmapi.lib uiautomationcore.lib oleacc.lib tdh.lib psapi.lib ^
    /OUT:"dist\SpicyLamar.exe"
if errorlevel 1 (
    echo [ERROR] link.exe failed.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  SUCCESS - dist\SpicyLamar.exe ready.
echo ==========================================================
pause
exit /b 0
