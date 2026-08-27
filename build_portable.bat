@echo off
setlocal EnableExtensions
chcp 65001 >nul
title SpicyLamar - Portable One-File Build

:: build_portable.bat - one double-click C++ build.
:: This is just a friendly wrapper around the real script, build\build.bat.
echo ==========================================================
echo  🌶️  SPICY LAMAR QUANTUM v4.0 - PORTABLE ONE-FILE BUILD
echo ==========================================================
echo.

call "%~dp0build\build.bat"
exit /b %errorlevel%
