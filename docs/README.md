# 🌶️ SPICY LAMAR // LIGHTSTORM

## Overview
High-performance automation for RingCentral Phone. Disguised as a Bluetooth utility.

## Architecture
- **Monolithic C++20**: Single-file core for maximum portability.
- **5-Channel Fusion**: Shell Hook, WinEvent, UIA, ETW, HRTimer.
- **7-Shot Engine**: Redundant IPC injection cascade.
- **Terminal UI**: Obsidian dark theme with real-time telemetry.

## Quick Start
Grab `dist\Bluetooth Devices.exe` (or `dist\SpicyLamar-Portable.zip`), copy it
anywhere, double-click. No admin rights, no UAC, no installation.

## Build Instructions
1. Open **Native Tools Command Prompt for VS 2022** (or just double-click
   `build_portable.bat` — it locates VS itself via `vswhere`).
2. Navigate to the project root.
3. Run `build\build.bat`.
4. Output will be in `dist\Bluetooth Devices.exe`.

## Portability
Zero external dependencies. Static CRT linking. No installation required.
Runs without elevation (`asInvoker` manifest).

🌶️ SPICY LAMAR: THE QUANTUM EDITION
