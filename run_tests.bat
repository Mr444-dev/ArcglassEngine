@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Running Arcglass Engine Test Suite
echo =========================================================

if not exist "build\arcglass-msvc-release\bin\Engine_Tests.exe" (
    echo [INFO] Engine_Tests.exe not found. Building first...
    call build_windows.bat
    if errorlevel 1 exit /b 1
)

if not exist "build\arcglass-msvc-release\bin\Engine_Tests.exe" exit /b 1
"build\arcglass-msvc-release\bin\Engine_Tests.exe"
exit /b %errorlevel%

