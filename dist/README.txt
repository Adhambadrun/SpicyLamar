SPICY LAMAR v4.2 - Portable Edition
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

ANSWER BEHAVIOR (ALWAYS-ON ATTENTION, default)
  * Hunts for the RingCentral Phone window 50x per second.
  * While it exists: focuses it (restores if minimized) + fires the
    Alt+F1 cascade, one every 0.5 s — never goes quiet.
  * F11 pauses/starts the engine at any time.
  * Optional bounded build (-DSPICY_LAMAR_BOUNDED): fires only on call
    activity, max 3 attempts per ringing episode, 1.5 s apart.

NOTES
  * Windows 10 / 11 x64. Single file, zero external dependencies.
  * If SmartScreen warns on first run: More info -> Run anyway
    (or right-click the exe -> Properties -> Unblock).
  * To rebuild from source, see build_portable.bat in the repo.
