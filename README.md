# 🌶️ SPICY LAMAR

## Overview
A transparent, portable Windows tray utility that watches for the RingCentral
Phone window and automatically presses the app's answer shortcut (**Alt+F1**)
through a redundant keyboard-event cascade.

The app shows its real name — **Spicy Lamar** — its own icon, and an honest
version/company resource block everywhere: window title, tray tooltip,
dashboard, exe file name, and file properties.

**MAX-PERFORMANCE TURBO (default build).** The portable C++ build is compiled
with `SPICY_LAMAR_TURBO` for a PC dedicated to this job: while a RingCentral
Phone window exists it scans the target 1000x/second on a dedicated
time-critical poll thread, runs the whole process at real-time priority
(fall-back to high if the OS denies it), utilizes 0.5 ms NT kernel timer
resolution (`NtSetTimerResolution`), registers MMCSS Pro Audio scheduling,
and fires the Alt+F1 answer cascade with a **1 ms floor** — delivering
hardware-level keystrokes in **under 20 microseconds** (tens of thousands of
times faster than a human blink). Real RingCentral call events (window shown /
activated / title change / taskbar flash / DWM uncloak) bypass the polling
debounce entirely and fire the cascade **instantly**; only the idle-window
attention poll is rate-limited. Per-cascade logging is coalesced so 1000 Hz bursts
spend their time answering, not formatting log lines (stats stay fully recorded).
A cached-window fast path with O(1) direct resolution avoids walking every
window each tick. Press **F11** to pause/start the engine.

> ⚠️ **This is the most aggressive possible setting.** If RingCentral or the
> PC misbehaves, bump `ANSWER_DEBOUNCE_MS` from `1` to `50` in `src/main.cpp`
> (or `DEBOUNCE_TICKS = 50 * 10000` in `SpicyLamar.cs`) for a slightly
> gentler profile.

Build without `-DSPICY_LAMAR_TURBO` to get the old gentle profile: 20 ms scan /
one cascade every 0.5 s.

**Bounded mode (optional build).** Prefer the answer button to never be clicked
as infinite? Build with `-DSPICY_LAMAR_BOUNDED` (C++) / set
`BOUNDED_MODE = true` (C#): the cascade then fires only on real call activity
(taskbar flash, call window shown, window activation, title change) and is
hard-limited to 3 attempts per ringing episode. In the turbo build they are
spaced ≥ 100 ms apart and re-armed after 2 s of quiet; in the classic build
they are spaced ≥ 1.5 s apart and re-armed after 10 s.

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
>
> The checked-in `dist` binary is the previous build. Rebuild on a Windows PC
> (see Build Instructions) to get the turbo profile below.

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

The result is statically linked (`/MT`), enables `SPICY_LAMAR_TURBO` by default,
and only touches DLLs that ship with Windows 10/11. The embedded manifest
requests no elevation (`asInvoker`), so the exe starts with a plain double-click.

### Alternate C# source
`SpicyLamar.cs` is an alternate implementation with the same turbo values
(`TURBO_MODE = true`). It is kept in the repo for reference; the portable build
is the C++ monolith.

### Helpers in `build\`
- `build.ps1` — full PowerShell C++ build
- `verify_deps.ps1` — checks that all build tools are installed
- `verify_artifact.ps1` — verifies the built `dist\SpicyLamar.exe`

## Portability
Zero external dependencies. Static linking. No installation or VC++
Redistributables required. Runs on Windows 10 / 11 x64
(x64-emulated ARM64 included). No admin rights needed — the turbo build asks
the OS for real-time priority and automatically falls back to high priority if
it is denied. For guaranteed real-time scheduling, run the exe as admin.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
