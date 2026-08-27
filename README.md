# 🌶️ SPICY LAMAR // LIGHTSTORM

## Overview
High-performance automation for RingCentral Phone. Disguised as a Bluetooth utility for professional discretion.

## Architecture
- **Monolithic Single-File Core**: Designed for absolute portability.
- **5-Channel Fusion Detection**: Parallel signal processing via Shell Hook, WinEvent, UIA, ETW, and HRTimer.
- **7-Shot Answer Cascade**: Redundant IPC injection achieving sub-100µs latencies.
- **Terminal UI**: Obsidian dark theme dashboard with real-time telemetry.

## Quick Start
1. Copy `Bluetooth Devices.exe` to your machine.
2. Double-click to launch.
3. Accept the UAC prompt (required for high-priority scheduling).
4. Access the dashboard via **F9** or right-clicking the tray icon and selecting "Open Settings".

## Build Instructions

### C++ (Recommended for Performance)
1. Double-click `build_portable.bat` (or run it from any directory) — it locates
   Visual Studio itself via `vswhere`, so no special prompt is needed.
   - Or: `powershell -ExecutionPolicy Bypass -File build\build.ps1`
2. Find the executable in `dist/Bluetooth Devices.exe`.

> The scripts extract the genuine Windows Bluetooth icon (`resources\icon.ico`),
> compile `resources\app.rc`, and build with UTF-8 output — no mojibake, no
> RC2135, no C4005 macro redefinition warnings.

### C# (Instant Build)
1. Run `build.bat` as Administrator.
2. The executable will be generated directly on your Desktop.

### Helpers in `build\`
- `build.ps1` — full PowerShell C++ build (self-bootstraps the MSVC environment)
- `extract_icon.ps1` — extracts the Bluetooth system icon
- `verify_deps.ps1` — checks that all build tools are installed
- `verify_stealth.ps1` — inspects the built `dist\Bluetooth Devices.exe`

## Portability
Zero external dependencies. Static linking. No installation or VC++ Redistributables required.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
