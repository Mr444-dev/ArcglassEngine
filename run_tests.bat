@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Running MyGameEngine Comprehensive Test Suite
echo =========================================================

if not exist "build\bin\Engine_Tests.exe" (
    echo [INFO] Engine_Tests.exe not found. Building first...
    call build_windows.bat
)

"build\bin\Engine_Tests.exe"
pause

