# 🌶️ SPICY LAMAR

## Overview
A transparent, portable Windows tray utility that watches for the RingCentral
Phone window and automatically presses the app's answer shortcut
(**Alt+F1 only** — RingCentral's stock Alt+A answer shortcut is never sent)
through a redundant keyboard-event cascade.

The app shows its real name — **Spicy Lamar** — its own icon, and an honest
version/company resource block everywhere: window title, tray tooltip,
dashboard, exe file name, and file properties.

**MAX-PERFORMANCE TURBO (default build).** The portable C++ build is compiled
with `SPICY_LAMAR_TURBO` for a PC dedicated to this job: while a RingCentral
Phone window exists it scans the target 200x/second on a dedicated
high-priority poll thread, runs the process at high priority, utilizes 0.5 ms
NT kernel timer resolution (`NtSetTimerResolution`), registers MMCSS Pro Audio
scheduling, and fires the Alt+F1 answer cascade with a **100 ms poll floor** —
delivering hardware-level keystrokes in **microseconds** (thousands of times
faster than a human blink). Real RingCentral call events (window shown /
activated / title change / taskbar flash / DWM uncloak) skip the poll debounce
and fire the cascade **instantly**; only back-to-back event storms are
coalesced (50 ms absolute floor) so the first ring event is never delayed.
Per-cascade logging is coalesced so bursts spend their time answering, not
formatting log lines (stats stay fully recorded).
A cached-window fast path with O(1) direct resolution avoids walking every
window each tick. Press **F11** to pause/start the engine.

> ✅ **v4.3 stability fix.** Earlier builds fired the cascade up to
> 1000x/second at real-time priority, which made RingCentral lag — and a
> `WM_SYSCHAR(VK_F1)` cascade step whose wParam (0x70) is ASCII `'p'` made
> RingCentral literally type `pppppppp`. Both are fixed: the bogus SysChar
> step is removed, the process runs at high (not real-time) priority, and
> every fire channel is rate-limited.
>
> ⚠️ If RingCentral or the PC still misbehaves, raise `ANSWER_DEBOUNCE_MS`
> further (e.g. `100` → `500`) in `src/main.cpp` (or `DEBOUNCE_TICKS = 500 *
> 10000` in `SpicyLamar.cs`) for a gentler profile.
>
> ✅ **v4.4 — Alt+F1 only + F8 self-test.** The answer key is **Alt+F1 only**;
> the stock RingCentral **Alt+A** shortcut is deliberately never synthesized
> (this build targets a RingCentral install remapped to Alt+F1, and a stray
> Alt+A could open a menu). Press **F8** (or tray → **Self-test (F8)**) to run
> an end-to-end self-test that finds the RingCentral window and fires one
> Alt+F1 cascade, reporting the result in the dashboard log — it works while
> paused and does not affect call stats.
>
> ✅ **v4.5 — fixed auto-answer delivery.** Auto-answer now delivers the Alt+F1
> cascade to **every** RingCentral window (a ringing call's answer UI is usually a
> separate popup window, not a child of the main window) and **no longer steals
> focus** off the foreground call popup — that was why a manual Alt+F1 answered but
> the auto-answer didn't. Detection also matches by RingCentral's **process name**,
> so it survives RingCentral title/class changes. When the engine is ACTIVE but a
> call isn't answered, open the dashboard (**F9**): it now logs
> `RingCentral window NOT found` if detection is the problem.

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
| **F8**  | **Self-test** — verify the Alt+F1 answer path (works while paused; doesn't change call stats) |
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
(x64-emulated ARM64 included). No admin rights needed — as of v4.3 the turbo
build runs at high (not real-time) priority, so it answers fast without
starving RingCentral's own threads.

---
🌶️⚡🔥 **SPICY LAMAR: THE QUANTUM EDITION** 🔥⚡🌶️
