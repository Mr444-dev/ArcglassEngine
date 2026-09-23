@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Launching MyGameEngine - Modern 3D Game Runtime
echo =========================================================

if not exist "build\bin\GameRuntime.exe" (
    echo [INFO] GameRuntime.exe not found. Building first...
    call build_windows.bat
)

start "" "build\bin\GameRuntime.exe"

