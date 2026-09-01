SPICY LAMAR v4.3 - Portable Edition
=============================================================

This bundle contains ONE file that matters:
"SpicyLamar.exe".

USAGE
  1. Copy "SpicyLamar.exe" anywhere (USB stick, Desktop, anywhere).
  2. Double-click it. No install, no admin, no UAC, no DLLs needed.
  3. It lives in the tray (named "Spicy Lamar") and answers RingCentral
     Phone calls automatically by pressing the app's answer shortcut
     (Alt+F1).

CONTROLS
  F9            Show/Hide dashboard
  F11           Pause/Start answering
  F12           Exit
  Right-click tray icon for the app menu.

ANSWER BEHAVIOR (MAX-PERFORMANCE TURBO, default)
  * Hunts for the RingCentral Phone window 200x per second (5 ms) on a
    dedicated high-priority poll thread.
  * Process runs at high priority (v4.3: high, not real-time, so
    RingCentral no longer lags).
  * While it exists: focuses it (restores if minimized) + fires the
    Alt+F1 cascade with a 100 ms poll floor.
  * Real call events (window shown/activated/title change) skip the
    polling debounce and fire the cascade instantly; back-to-back
    event storms are coalesced at a 50 ms absolute floor.
  * v4.3 fix: removed the WM_SYSCHAR(VK_F1) cascade step whose wParam
    (0x70 = ASCII 'p') made RingCentral literally type 'pppppppp'.
  * Per-cascade logging is coalesced at high rates; stats stay fully
    recorded.
  * If RingCentral or the PC still misbehaves, rebuild with a higher
    ANSWER_DEBOUNCE_MS (e.g. 500) in src\main.cpp for a gentler profile.
  * F11 pauses/starts the engine at any time.
  * Classic profile (build without SPICY_LAMAR_TURBO): 50x/sec scan,
    one cascade every 0.5 s.
  * Optional bounded build (-DSPICY_LAMAR_BOUNDED): fires only on call
    activity, max 3 attempts per ringing episode — turbo: 100 ms apart /
    2 s re-arm; classic: 1.5 s apart / 10 s re-arm.

NOTES
  * Windows 10 / 11 x64. Single file, zero external dependencies.
  * If SmartScreen warns on first run: More info -> Run anyway
    (or right-click the exe -> Properties -> Unblock).
  * To rebuild from source, see build_portable.bat in the repo.
