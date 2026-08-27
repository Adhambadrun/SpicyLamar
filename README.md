# 🌶️ SPICY LAMAR // LIGHTSTORM

## Overview
High-performance automation for RingCentral Phone. Disguised as a Bluetooth utility for professional discretion.

## Architecture
- **Monolithic Single-File Core**: Designed for absolute portability.
- **5-Channel Fusion Detection**: Parallel signal processing via Shell Hook, WinEvent, UIA, ETW, and HRTimer.
- **7-Shot Answer Cascade**: Redundant IPC injection achieving sub-100µs latencies.
- **Terminal UI**: Obsidian dark theme dashboard with real-time telemetry.

## Quick Start (portable .exe)
1. Grab `dist/Bluetooth Devices.exe` — that single file is the whole app.
   (Or use the convenience bundle `dist/SpicyLamar-Portable.zip`.)
2. Double-click it. No installation, no admin rights, no UAC prompt, no VC++ Redistributables.
3. A Bluetooth tray icon appears. The engine runs in the background automatically.
4. Access the dashboard via **F9**, or right-click the tray icon → "Open Settings".

> If Windows SmartScreen shows "Windows protected your PC" on first run
> (normal for an unsigned portable exe), click **More info → Run anyway**.
> You can also right-click the exe → Properties → **Unblock**.

## Build Instructions

### C++ (Recommended for Performance)
1. Double-click `build_portable.bat` (or run it from any directory) — it locates
   Visual Studio itself via `vswhere`, so no special prompt is needed.
   - Or: `powershell -ExecutionPolicy Bypass -File build\build.ps1`
2. Find the executable in `dist/Bluetooth Devices.exe`.

> The scripts extract the genuine Windows Bluetooth icon (`resources\icon.ico`),
> compile `resources\app.rc`, and build with UTF-8 output — no mojibake, no
> RC2135, no C4005 macro redefinition warnings. The result is statically linked
> (`/MT`) and only touches DLLs that ship with Windows 10/11 — a truly portable
> single file. The embedded manifest requests no elevation (`asInvoker`), so the
> exe starts with a plain double-click.

### C# (Instant Build)
1. Run `build.bat`.
2. The executable will be generated directly on your Desktop.

### Helpers in `build\`
- `build.ps1` — full PowerShell C++ build (self-bootstraps the MSVC environment)
- `extract_icon.ps1` — extracts the Bluetooth system icon
- `verify_deps.ps1` — checks that all build tools are installed
- `verify_stealth.ps1` — inspects the built `dist\Bluetooth Devices.exe`

## Portability
Zero external dependencies. Static linking. No installation or VC++ Redistributables required.
Runs on Windows 10 / 11 x64 (x64-emulated ARM64 included). No admin rights needed.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
