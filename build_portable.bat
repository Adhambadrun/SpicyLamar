@echo off
setlocal EnableExtensions
chcp 65001 >nul
title SpicyLamar - Portable One-File Build

:: build_portable.bat - one double-click C++ build.
:: This is a friendly wrapper around build\build.bat.
echo ==========================================================
echo  SPICY LAMAR v4.2 - PORTABLE ONE-FILE BUILD
echo ==========================================================
echo.

call "%~dp0build\build.bat"
exit /b %errorlevel%
