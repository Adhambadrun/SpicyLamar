# 🌶️ SPICY LAMAR // RingCentral Auto-Answer

## Overview
A transparent, portable Windows tray utility that detects an active
**RingCentral Phone** window and automatically presses the app's answer
shortcut (Alt+F1) through a redundant keyboard-event cascade.

The app is **not** disguised as a Bluetooth utility and does not impersonate
Microsoft or Windows components. It shows its real name, its own icon, and an
honest version/company resource block.

## Quick Start (portable .exe)
1. Grab `dist\SpicyLamar RingCentral Auto-Answer.exe` (single file, portable)
   or `dist\SpicyLamar-Portable.zip`.
2. Double-click it. No installation, no admin rights, no UAC prompt,
   no VC++ Redistributables.
3. A Spicy Lamar tray icon appears. The engine runs in the background.
4. Open the dashboard with **F9**, or right-click the tray icon →
   **Open Dashboard**.

> If Windows SmartScreen shows "Windows protected your PC" on first run
> (normal for an unsigned portable exe), click **More info → Run anyway**.
> You can also right-click the exe → Properties → **Unblock**.

## Build Instructions

### C++ (Recommended for Performance)
1. Double-click `build_portable.bat`.
   - Or: `powershell -ExecutionPolicy Bypass -File build\build.ps1`
2. Find the executable in `dist\SpicyLamar RingCentral Auto-Answer.exe`.

The result is statically linked (`/MT`) and only touches DLLs that ship with
Windows 10/11. The embedded manifest requests no elevation (`asInvoker`), so
the exe starts with a plain double-click.

### C# (Instant Build)
1. Run `build.bat`.
2. Find `dist\SpicyLamar RingCentral Auto-Answer.exe`.

### Helpers in `build\`
- `build.ps1` — full PowerShell C++ build
- `verify_deps.ps1` — checks that all build tools are installed
- `verify_artifact.ps1` — verifies the built `dist\SpicyLamar RingCentral Auto-Answer.exe`

## Portability
Zero external dependencies. Static linking. No installation or VC++
Redistributables required. Runs on Windows 10 / 11 x64
(x64-emulated ARM64 included). No admin rights needed.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
