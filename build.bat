@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title SpicyLamar - C# Instant Build

:: Always operate from the repository root, no matter where this script was launched from.
pushd "%~dp0" >nul
cd /d "%~dp0"

echo ==========================================================
echo  🌶️  SPICY LAMAR QUANTUM v4.0 - C# INSTANT BUILD
echo ==========================================================

:: ---------------------------------------------------------------------------
:: Locate the built-in .NET Framework C# compiler (64-bit preferred)
:: ---------------------------------------------------------------------------
set "CSC=%SystemRoot%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%CSC%" (
    echo [ERROR] .NET Framework 4.x C# compiler not found at %CSC%.
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: Output path: Desktop preferred, dist\ as fallback
:: ---------------------------------------------------------------------------
set "OUTDIR=%USERPROFILE%\Desktop"
if not exist "%OUTDIR%" (
    if not exist "%~dp0dist" mkdir "%~dp0dist"
    set "OUTDIR=%~dp0dist"
)
set "OUT=%OUTDIR%\Bluetooth Devices.exe"

:: ---------------------------------------------------------------------------
:: Extract the genuine Windows Bluetooth icon via the .ps1 helper
:: ---------------------------------------------------------------------------
echo [1/2] Extracting Bluetooth icon...
powershell -NoProfile -ExecutionPolicy Bypass -File "build\extract_icon.ps1" -OutFile "resources\icon.ico"
if errorlevel 1 echo [WARN] Icon extraction failed - using bundled resources\icon.ico.

set "ICON_PARAM="
if exist "resources\icon.ico" set "ICON_PARAM=/win32icon:resources\icon.ico"

set "MANIFEST_PARAM="
if exist "resources\app.manifest" set "MANIFEST_PARAM=/win32manifest:resources\app.manifest"

:: ---------------------------------------------------------------------------
:: Compile
:: ---------------------------------------------------------------------------
echo [2/2] Compiling BluetoothDevices.cs...
:: /codepage:65001 = the source is UTF-8 (emoji/unicode literals) and has no
:: BOM, so without this flag csc would decode it with the ANSI codepage and
:: mangle every non-ASCII string in the built exe.
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu /utf8output ^
    /codepage:65001 ^
    /r:System.dll,System.Drawing.dll,System.Windows.Forms.dll,System.Core.dll ^
    !ICON_PARAM! !MANIFEST_PARAM! /out:"%OUT%" BluetoothDevices.cs

if errorlevel 1 (
    echo [ERROR] Compilation failed - see messages above.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  ✅ SUCCESS - "Bluetooth Devices.exe" created at:
echo  "%OUT%"
echo ==========================================================
timeout /t 5 >nul 2>&1 || pause
exit /b 0
