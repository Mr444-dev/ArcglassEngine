@echo off
setlocal
cd /d "%~dp0"

echo =========================================================
echo    Arcglass Engine Launcher
echo =========================================================
echo.

if not exist "build\arcglass-msvc-release\bin\EngineEditor.exe" (
    echo [INFO] Binaries not found. Building engine first...
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

if "%1"=="game" (
    echo Launching the standalone runtime...
    if not exist "build\arcglass-msvc-release\bin\GameRuntime.exe" exit /b 1
    start "" "build\arcglass-msvc-release\bin\GameRuntime.exe"
    goto :eof
)

if "%1"=="tests" (
    echo Running Engine Test Suite...
    if not exist "build\arcglass-msvc-release\bin\Engine_Tests.exe" exit /b 1
    "build\arcglass-msvc-release\bin\Engine_Tests.exe"
    if errorlevel 1 exit /b 1
    exit /b 0
)

echo Launching the editor...
start "" "build\arcglass-msvc-release\bin\EngineEditor.exe"

