@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Launching Arcglass Engine Editor
echo =========================================================

if not exist "build\arcglass-msvc-release\bin\EngineEditor.exe" (
    echo [INFO] EngineEditor.exe not found. Building first...
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

if not exist "build\arcglass-msvc-release\bin\EngineEditor.exe" exit /b 1
start "" "build\arcglass-msvc-release\bin\EngineEditor.exe"

