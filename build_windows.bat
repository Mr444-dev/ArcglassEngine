@echo off
setlocal
cd /d "%~dp0"
echo =========================================================
echo    Configuring & Building MyGameEngine (DirectX 12 / C++20)
echo =========================================================

where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
)

cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_EDITOR=ON -DBUILD_TESTS=ON
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

cmake --build build --config RelWithDebInfo --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Engine build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] MyGameEngine built successfully!
pause

