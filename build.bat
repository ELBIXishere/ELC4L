@echo off
setlocal EnableDelayedExpansion

echo === ELC4L Build Script (32-bit and 64-bit) ===
echo.

cd /d "%~dp0"

:: Find VS2022 Build Tools
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%i
)

if not defined VSINSTALL (
    set "VSINSTALL=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
)

set "VCVARSALL=%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARSALL%" (
    echo Cannot find Visual Studio Build Tools!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Building 64-bit version...
echo ========================================

call "%VCVARSALL%" x64

echo Cleaning build64 directory...
if exist build64 rmdir /s /q build64
mkdir build64
cd build64

echo Running CMake for x64...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed for x64!
    cd ..
    pause
    exit /b 1
)

echo Building Release x64...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed for x64!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Building 32-bit version...
echo ========================================

:: Reinitialize for x86
call "%VCVARSALL%" x86

echo Cleaning build32 directory...
if exist build32 rmdir /s /q build32
mkdir build32
cd build32

echo Running CMake for Win32...
cmake .. -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 (
    echo CMake configuration failed for Win32!
    cd ..
    pause
    exit /b 1
)

echo Building Release Win32...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed for Win32!
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Build successful!
echo.
echo 64-bit DLL: build64\bin\Release\ELC4L.dll
echo 32-bit DLL: build32\bin\Release\ELC4L.dll
echo ========================================
pause
