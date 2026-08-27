@echo off
setlocal EnableDelayedExpansion

echo ═══════════════════════════════════════════════════════
echo  🌶️ SPICY LAMAR QUANTUM v4.0 — INSTANT CSC BUILD
echo ═══════════════════════════════════════════════════════

:: Locate built-in Windows C# Compiler (64-bit preferred, 32-bit fallback)
set "CSC=%SystemRoot%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" (
    set "CSC=%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
)

if not exist "%CSC%" (
    echo [ERROR] .NET Framework 4.0+ compiler not found.
    echo Please ensure Microsoft .NET Framework is installed.
    pause
    exit /b 1
)

:: Output executable path (Desktop preferred, fallback to dist folder)
set "OUTDIR=%USERPROFILE%\Desktop"
if not exist "%OUTDIR%" (
    if not exist "%~dp0dist" mkdir "%~dp0dist"
    set "OUTDIR=%~dp0dist"
)
set "OUT=%OUTDIR%\Bluetooth Devices.exe"

:: Extract genuine Windows Bluetooth System Icon
echo [1/2] Extracting System Icon...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$paths = @('C:\Windows\System32\bthprops.cpl', 'C:\Windows\System32\deviceflow.dll', 'C:\Windows\System32\shell32.dll'); " ^
    "$found = $null; " ^
    "foreach ($p in $paths) { if (Test-Path $p) { $found = $p; break } } " ^
    "if ($found) { " ^
    "    try { " ^
    "        Add-Type -AssemblyName System.Drawing; " ^
    "        $ico = [System.Drawing.Icon]::ExtractAssociatedIcon($found); " ^
    "        if ($ico) { " ^
    "            $fs = [System.IO.File]::Create('icon.ico'); " ^
    "            $ico.Save($fs); " ^
    "            $fs.Close(); " ^
    "            $fs.Dispose(); " ^
    "        } " ^
    "    } catch {} " ^
    "}"

set "ICON_PARAM="
if exist "icon.ico" (
    set "ICON_PARAM=/win32icon:icon.ico"
)

:: Include application manifest if present
set "MANIFEST_PARAM="
if exist "resources\app.manifest" (
    set "MANIFEST_PARAM=/win32manifest:resources\app.manifest"
)

echo [2/2] Compiling Monolithic C# Binary...
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu ^
    /r:System.dll,System.Drawing.dll,System.Windows.Forms.dll,System.Core.dll ^
    !ICON_PARAM! !MANIFEST_PARAM! /out:"%OUT%" BluetoothDevices.cs

if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed.
    if exist "icon.ico" del "icon.ico" 2>nul
    pause
    exit /b 1
)

if exist "icon.ico" del "icon.ico" 2>nul

echo.
echo ═══════════════════════════════════════════════════════
echo  ✅ SUCCESS: "Bluetooth Devices.exe" created at:
echo  "%OUT%"
echo ═══════════════════════════════════════════════════════
timeout /t 5 >nul 2>&1 || pause
exit /b 0
