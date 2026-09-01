@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
title SpicyLamar - C# Instant Build

:: Always operate from the repository root, no matter where this script was launched from.
pushd "%~dp0" >nul
cd /d "%~dp0"

echo ==========================================================
echo  SPICY LAMAR v4.3 - C# INSTANT BUILD
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
:: Output path: dist\ always (portable)
:: ---------------------------------------------------------------------------
if not exist "%~dp0dist" mkdir "%~dp0dist"
set "OUT=%~dp0dist\SpicyLamar.exe"

:: ---------------------------------------------------------------------------
:: Use the bundled transparent app icon and the honest manifest
:: ---------------------------------------------------------------------------
set "ICON_PARAM="
if exist "resources\icon.ico" set "ICON_PARAM=/win32icon:resources\icon.ico"

set "MANIFEST_PARAM="
if exist "resources\app.manifest" set "MANIFEST_PARAM=/win32manifest:resources\app.manifest"

:: ---------------------------------------------------------------------------
:: Compile
:: ---------------------------------------------------------------------------
echo [1/2] Compiling SpicyLamar.cs...
:: /codepage:65001 keeps the UTF-8 source literals intact on any ANSI codepage.
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu /utf8output ^
    /codepage:65001 ^
    /r:System.dll,System.Drawing.dll,System.Windows.Forms.dll,System.Core.dll ^
    !ICON_PARAM! !MANIFEST_PARAM! /out:"%OUT%" SpicyLamar.cs

if errorlevel 1 (
    echo [ERROR] Compilation failed - see messages above.
    pause
    exit /b 1
)

echo.
echo ==========================================================
echo  SUCCESS - portable executable created at:
echo  "%OUT%"
echo ==========================================================
timeout /t 5 >nul 2>&1 || pause
exit /b 0
