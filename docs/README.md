# 🌶️ SPICY LAMAR

## Overview
A portable Windows tray utility that automatically answers RingCentral Phone
calls by detecting call activity in the RingCentral app window and sending the
app's answer shortcut (Alt+F1) through a redundant keyboard cascade.

The answer cascade is **bounded**: max 3 attempts per ringing episode, spaced
≥ 1.5 s apart — it never clicks infinitely, and never fires at an idle window.

## Quick Start
Grab `dist\SpicyLamar.exe` (or `dist\SpicyLamar-Portable.zip`), copy it
anywhere, and double-click. No admin rights, no UAC, no installation.

## Build Instructions
1. Open **Native Tools Command Prompt for VS 2022** (or double-click
   `build_portable.bat` — it locates VS itself via `vswhere`).
2. Navigate to the project root.
3. Run `build\build.bat`.
4. Output will be in `dist\SpicyLamar.exe`.

## Portability
Zero external dependencies. Static CRT linking. No installation required.
Runs without elevation (`asInvoker` manifest).
