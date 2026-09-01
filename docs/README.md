# 🌶️ SPICY LAMAR

## Overview
A portable Windows tray utility that automatically answers RingCentral Phone
calls by watching for the RingCentral app window and sending the app's answer
shortcut (Alt+F1) through a redundant keyboard cascade.

**MAX-PERFORMANCE (default portable build, `SPICY_LAMAR_TURBO`):** while a
RingCentral Phone window exists, the engine scans 200x/second on a
high-priority worker thread with MMCSS Pro Audio scheduling and 0.5 ms kernel
timer resolution, firing the Alt+F1 cascade with a **100 ms poll floor**
(microsecond cascade execution latency — thousands of times faster
than a blink), with high (not real-time, v4.3) process priority — for a PC
dedicated to this job. Real call events skip the polling debounce and fire the
cascade instantly (event storms are coalesced at a 50 ms absolute floor);
per-cascade logging is coalesced at high rates (stats stay fully
recorded). ✅ v4.3 removed the `WM_SYSCHAR(VK_F1)` cascade step whose wParam
(0x70 = ASCII `'p'`) made RingCentral type `pppppppp`, and fixed the
RingCentral lag caused by real-time priority + unlimited cascade rates.
Build without `SPICY_LAMAR_TURBO` for the classic gentle profile
(20 ms scan / 0.5 s).
**Bounded mode (optional build):** fires only on call activity, max 3 attempts
per ringing episode — ≥ 100 ms apart / 2 s re-arm in turbo, or ≥ 1.5 s apart /
10 s re-arm in the classic profile.

## Quick Start
Grab `dist\SpicyLamar.exe` (or `dist\SpicyLamar-Portable.zip`), copy it
anywhere, and double-click. No admin rights, no UAC, no installation.

> The checked-in `dist` binary is the previous build. Rebuild on a Windows PC
> to get the turbo profile below.

## Build Instructions
1. Open **Native Tools Command Prompt for VS 2022** (or double-click
   `build_portable.bat` — it locates VS itself via `vswhere`).
2. Navigate to the project root.
3. Run `build\build.bat`.
4. Output will be in `dist\SpicyLamar.exe`.

## Portability
Zero external dependencies. Static CRT linking. No installation required.
Runs without elevation (`asInvoker` manifest).
