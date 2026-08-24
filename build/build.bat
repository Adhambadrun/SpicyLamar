@echo off
setlocal EnableDelayedExpansion

echo ═══════════════════════════════════════════════════════
echo  🌶️ SPICY LAMAR QUANTUM v4.0 (LIGHTSTORM) — BUILD
echo ═══════════════════════════════════════════════════════

where cl.exe >nul 2>nul
if errorlevel 1 (
    echo [ERROR] MSVC cl.exe not found. Please run from a Native Tools Command Prompt.
    exit /b 1
)

if not exist build\obj mkdir build\obj
if not exist dist mkdir dist

echo [1/4] Extracting Bluetooth icon...
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (!(Test-Path resources\icon.ico)) { Add-Type -AssemblyName System.Drawing; $icon = [System.Drawing.Icon]::ExtractAssociatedIcon('C:\Windows\System32\bthprops.cpl'); $fs = [System.IO.File]::OpenWrite('resources\icon.ico'); $icon.Save($fs); $fs.Close(); }"

echo [2/4] Compiling resources...
rc.exe /nologo /fo build\obj\app.res resources\app.rc

echo [3/4] Compiling and Linking Monolith...
cl.exe /nologo /std:c++20 /O2 /Oi /GL /Gy /MT /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DSPICY_LAMAR_QUANTUM /EHsc src\main.cpp /Fo:build\obj\ /link /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,10.0 /MACHINE:X64 build\obj\app.res comctl32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib uxtheme.lib winmm.lib avrt.lib dwmapi.lib uiautomationcore.lib oleacc.lib tdh.lib psapi.lib /OUT:"dist\Bluetooth Devices.exe"

echo [4/4] Compiling Benchmark...
cl.exe /nologo /std:c++20 /O2 /MT /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DSPICY_LAMAR_QUANTUM /EHsc tests\benchmark.cpp /Fo:build\obj\benchmark.obj /link /SUBSYSTEM:CONSOLE,10.0 /MACHINE:X64 comctl32.lib shell32.lib ole32.lib oleaut32.lib advapi32.lib uxtheme.lib winmm.lib avrt.lib dwmapi.lib uiautomationcore.lib oleacc.lib tdh.lib psapi.lib /OUT:"dist\benchmark.exe"

echo.
echo ═══════════════════════════════════════════════════════
echo  ✅ BUILD SUCCESSFUL
echo  Artifact: dist\Bluetooth Devices.exe
echo ═══════════════════════════════════════════════════════
