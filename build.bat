@echo off
setlocal
echo ═══════════════════════════════════════════════════════
echo  🌶️ SPICY LAMAR QUANTUM v4.0 — INSTANT CSC BUILD
echo ═══════════════════════════════════════════════════════

:: Path to the built-in Windows C# Compiler
set "CSC=C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
set "OUT=%USERPROFILE%\Desktop\Bluetooth Devices.exe"

if not exist "%CSC%" (
    echo [ERROR] .NET Framework 4.0 not found.
    pause
    exit /b
)

echo [1/2] Extracting System Icon...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-Type -AssemblyName System.Drawing; [System.Drawing.Icon]::ExtractAssociatedIcon('C:\Windows\System32\bthprops.cpl').Save([System.IO.File]::OpenWrite('icon.ico'))"

echo [2/2] Compiling Monolithic C# Binary...
"%CSC%" /nologo /target:winexe /optimize+ /platform:anycpu /out:"%OUT%" /win32icon:icon.ico BluetoothDevices.cs

if errorlevel 1 (
    echo [ERROR] Compilation failed.
    pause
    exit /b
)

echo.
echo ═══════════════════════════════════════════════════════
echo  ✅ SUCCESS: "Bluetooth Devices.exe" created on Desktop.
echo ═══════════════════════════════════════════════════════
del icon.ico
timeout /t 5
exit
