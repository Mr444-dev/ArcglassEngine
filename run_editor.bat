@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Launching MyGameEngine - Standalone 3D Editor
echo =========================================================

if not exist "build\bin\EngineEditor.exe" (
    echo [INFO] EngineEditor.exe not found. Building first...
    call build_windows.bat
)

start "" "build\bin\EngineEditor.exe"

