@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Launching Arcglass Runtime
echo =========================================================

if not exist "build\arcglass-msvc-release\bin\GameRuntime.exe" (
    echo [INFO] GameRuntime.exe not found. Building first...
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

if not exist "build\arcglass-msvc-release\bin\GameRuntime.exe" exit /b 1
start "" "build\arcglass-msvc-release\bin\GameRuntime.exe"

