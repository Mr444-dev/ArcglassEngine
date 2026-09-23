@echo off
setlocal
cd /d "%~dp0"

echo =========================================================
echo    MyGameEngine - Standard 2026 Unified Launcher
echo =========================================================
echo.

if not exist "build\bin\EngineEditor.exe" (
    echo [INFO] Binaries not found. Building engine first...
    call build_windows.bat
)

if "%1"=="game" (
    echo Launching Standalone Game Runtime...
    start "" "build\bin\GameRuntime.exe"
    goto :eof
)

if "%1"=="tests" (
    echo Running Engine Test Suite...
    "build\bin\Engine_Tests.exe"
    pause
    goto :eof
)

echo Launching Unified 3D Engine Editor...
echo (Tip: To launch standalone game client directly, run: run.bat game)
start "" "build\bin\EngineEditor.exe"

