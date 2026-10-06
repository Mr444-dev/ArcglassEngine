@echo off
setlocal
cd /d "%~dp0"

set "BUILD_PRESET=windows-msvc-release"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found. Install CMake 3.28 or newer and add it to PATH.
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Ninja was not found. Install Ninja and add it to PATH.
    exit /b 1
)

where cl >nul 2>nul
if errorlevel 1 if not defined VSCMD_VER call :initialize_msvc

where cl >nul 2>nul
if errorlevel 1 (
    echo [ERROR] MSVC was not found. Install Visual Studio C++ Build Tools and the Windows SDK.
    exit /b 1
)

echo Configuring %BUILD_PRESET%...
cmake --preset %BUILD_PRESET%
if errorlevel 1 exit /b 1

echo Building Arcglass Engine...
cmake --build --preset %BUILD_PRESET% --parallel
set "BUILD_RESULT=%ERRORLEVEL%"
if not "%BUILD_RESULT%"=="0" (
    echo [ERROR] Build failed with exit code %BUILD_RESULT%.
    exit /b %BUILD_RESULT%
)

echo [SUCCESS] Build completed. Output: build\arcglass-msvc-release\bin
exit /b 0

:initialize_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 0
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL exit /b 0
if not exist "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" exit /b 0
call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
exit /b %errorlevel%
