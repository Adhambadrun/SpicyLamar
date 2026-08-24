@echo off
setlocal EnableDelayedExpansion

echo ═══════════════════════════════════════════════════════
echo  🌶️ SPICY LAMAR QUANTUM v4.0 — PORTABLE ONE-FILE BUILD
echo ═══════════════════════════════════════════════════════

:: Find MSVC
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSPATH=%%i"
)

if not defined VSPATH (
    echo [ERROR] Visual Studio not found. Please install VS 2022 Build Tools.
    pause
    exit /b 1
)

call "!VSPATH!\VC\Auxiliary\Build\vcvars64.bat"

if not exist build\obj mkdir build\obj
if not exist dist mkdir dist

echo [1/3] Extracting Bluetooth Resources...
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (!(Test-Path resources\icon.ico)) { Add-Type -AssemblyName System.Drawing; $icon = [System.Drawing.Icon]::ExtractAssociatedIcon('C:\Windows\System32\bthprops.cpl'); $fs = [System.IO.File]::OpenWrite('resources\icon.ico'); $icon.Save($fs); $fs.Close(); }"

echo [2/3] Compiling Metadata and Manifest...
rc.exe /nologo /fo build\obj\app.res resources\app.rc

echo [3/3] Generating High-Performance Monolith...
cl.exe /nologo /std:c++20 /O2 /Oi /GL /Gy /MT /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DSPICY_LAMAR_QUANTUM /EHsc src\main.cpp /Fo:build\obj\ /link /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,10.0 /MACHINE:X64 build\obj\app.res comctl32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib uxtheme.lib winmm.lib avrt.lib dwmapi.lib uiautomationcore.lib oleacc.lib tdh.lib psapi.lib /OUT:"dist\Bluetooth Devices.exe"

echo.
echo ═══════════════════════════════════════════════════════
echo  ✅ SUCCESS: dist\Bluetooth Devices.exe is ready.
echo ═══════════════════════════════════════════════════════
pause
