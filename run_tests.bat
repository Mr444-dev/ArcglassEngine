@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Running Arcglass Engine Test Suite
echo =========================================================

echo [INFO] Rebuilding the current CPU test target before execution...
call build_windows.bat
if errorlevel 1 exit /b 1

if not exist "build\arcglass-msvc-release\bin\Engine_Tests.exe" exit /b 1
"build\arcglass-msvc-release\bin\Engine_Tests.exe"
exit /b %errorlevel%

