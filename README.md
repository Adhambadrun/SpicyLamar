# 🌶️ SPICY LAMAR

## Overview
A transparent, portable Windows tray utility that watches for the RingCentral
Phone window and automatically presses the app's answer shortcut (**Alt+F1**)
through a redundant keyboard-event cascade.

The app shows its real name — **Spicy Lamar** — its own icon, and an honest
version/company resource block everywhere: window title, tray tooltip,
dashboard, exe file name, and file properties.

**ALWAYS-ON ATTENTION (default).** While a RingCentral Phone window exists,
Spicy Lamar relentlessly attends to it: it finds the window, focuses it
(restores it if minimized, brings it to the front) and fires the Alt+F1 answer
cascade — throttled to one cascade every 0.5 s. It never goes quiet on its own;
press **F11** to pause/start the engine. This is the classic v4.0 behavior.

**Bounded mode (optional build).** Prefer the answer button to never be clicked
as infinite? Build with `-DSPICY_LAMAR_BOUNDED` (C++) / set
`BOUNDED_MODE = true` (C#): the cascade then fires only on real call activity
(taskbar flash, call window shown, window activation, title change) and is
hard-limited to 3 attempts per ringing episode, spaced ≥ 1.5 s apart,
re-armed after 10 s of quiet.

## Quick Start (portable .exe)
1. Grab `dist\SpicyLamar.exe` (single file, portable)
   or `dist\SpicyLamar-Portable.zip`.
2. Double-click it. No installation, no admin rights, no UAC prompt,
   no VC++ Redistributables.
3. A Spicy Lamar tray icon appears. The engine runs in the background.
4. Open the dashboard with **F9**, or right-click the tray icon →
   **Open Dashboard (F9)**.

> If Windows SmartScreen shows "Windows protected your PC" on first run
> (normal for an unsigned portable exe), click **More info → Run anyway**.
> You can also right-click the exe → Properties → **Unblock**.

## Controls
| Key | Action |
| --- | ------ |
| **F9**  | Show / hide the dashboard |
| **F11** | **Pause / Start** the engine (also in the tray menu) |
| **F12** | Exit |
| **Alt+F1** | The answer shortcut Spicy Lamar sends for you |

Pause/Start is always visible: the dashboard status line, the tray tooltip
(`Spicy Lamar — PAUSED (F11 to start)`), and the tray menu label all track it,
and every toggle is written to the system log.

## Build Instructions

### C++ (Recommended for Performance)
1. Double-click `build_portable.bat`.
   - Or: `powershell -ExecutionPolicy Bypass -File build\build.ps1`
2. Find the executable in `dist\SpicyLamar.exe`.

The result is statically linked (`/MT`) and only touches DLLs that ship with
Windows 10/11. The embedded manifest requests no elevation (`asInvoker`), so
the exe starts with a plain double-click.

### C# (Instant Build)
1. Run `build.bat`.
2. Find the executable in `dist\SpicyLamar.exe`.

### Helpers in `build\`
- `build.ps1` — full PowerShell C++ build
- `verify_deps.ps1` — checks that all build tools are installed
- `verify_artifact.ps1` — verifies the built `dist\SpicyLamar.exe`

## Portability
Zero external dependencies. Static linking. No installation or VC++
Redistributables required. Runs on Windows 10 / 11 x64
(x64-emulated ARM64 included). No admin rights needed.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
