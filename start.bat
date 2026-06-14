@echo off
setlocal enabledelayedexpansion
title Spoti

echo.
echo  ==========================================
echo   SPOTI - Music Library
echo  ==========================================
echo.

:: ── Check g++ ──────────────────────────────────────────────
where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo  [ERROR] g++ not found in PATH.
    echo.
    echo  Install MinGW-w64 via MSYS2:
    echo    1. Download from https://www.msys2.org
    echo    2. Open MSYS2 UCRT64 terminal and run:
    echo         pacman -S mingw-w64-ucrt-x86_64-gcc
    echo    3. Add C:\msys64\ucrt64\bin to your system PATH
    echo    4. Open a NEW Command Prompt and run start.bat again
    echo.
    pause
    exit /b 1
)

:: ── Build ───────────────────────────────────────────────────
echo  [1/3] Building backend...
cd /d "%~dp0backend"

g++ -std=c++17 -O2 -Wall -o spoti-server.exe ^
    main.cpp db.cpp router.cpp -lws2_32

if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] Build failed. See errors above.
    pause
    exit /b 1
)
echo         Build OK.

:: ── Kill any old instance on port 8080 ─────────────────────
echo  [2/3] Checking port 8080...
for /f "tokens=5" %%P in ('netstat -ano 2^>nul ^| findstr /R "[:.]8080 "') do (
    taskkill /F /PID %%P >nul 2>&1
)
timeout /t 1 /nobreak >nul

:: ── Start server ────────────────────────────────────────────
echo  [3/3] Starting server...
start "Spoti Server" "%~dp0backend\spoti-server.exe" "%~dp0data\songs.csv"
timeout /t 2 /nobreak >nul
echo         Server started.

:: ── Open frontend in browser ────────────────────────────────
echo         Opening frontend...
start "" "%~dp0frontend\index.html"

echo.
echo  ==========================================
echo   Server  : http://localhost:8080
echo   API     : http://localhost:8080/songs
echo   Database: data\songs.csv
echo  ==========================================
echo.
echo  Press any key to STOP the server and exit.
echo.
pause >nul

:: ── Shutdown ────────────────────────────────────────────────
echo  Stopping server...
for /f "tokens=5" %%P in ('netstat -ano 2^>nul ^| findstr /R "[:.]8080 "') do (
    taskkill /F /PID %%P >nul 2>&1
)
echo  Done. Goodbye!
timeout /t 2 /nobreak >nul
