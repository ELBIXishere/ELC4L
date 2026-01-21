@echo off
setlocal
echo ============================================
echo ELBIX VST2 Plugin - Windows Build (x64 + x86)
echo ============================================
echo.

cd /d "%~dp0"

:: Initialize VS2022 Build Tools environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio environment!
    pause
    exit /b 1
)

REM ============================================
REM 64-bit Windows Build
REM ============================================
echo.
echo [1/2] Building 64-bit Windows DLL...
echo ============================================

if exist "build_x64" rmdir /s /q "build_x64"
mkdir "build_x64"
cd "build_x64"

cmake -G "Visual Studio 17 2022" -A x64 ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed for x64!
    cd ..
    pause
    exit /b 1
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed for x64!
    cd ..
    pause
    exit /b 1
)

cd ..
echo [SUCCESS] 64-bit build complete: build_x64\bin\Release\ELC4L.dll

REM ============================================
REM 32-bit Windows Build
REM ============================================
echo.
echo [2/2] Building 32-bit Windows DLL...
echo ============================================

if exist "build_x86" rmdir /s /q "build_x86"
mkdir "build_x86"
cd "build_x86"

cmake -G "Visual Studio 17 2022" -A Win32 ..
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed for x86!
    cd ..
    pause
    exit /b 1
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed for x86!
    cd ..
    pause
    exit /b 1
)

cd ..
echo [SUCCESS] 32-bit build complete: build_x86\bin\Release\ELC4L.dll

REM ============================================
REM Copy to output folder
REM ============================================
echo.
echo ============================================
echo Copying to output folder...
echo ============================================

if not exist "output" mkdir "output"
copy /Y "build_x64\bin\Release\ELC4L.dll" "output\ELBIX_x64.dll"
copy /Y "build_x86\bin\Release\ELC4L.dll" "output\ELBIX_x86.dll"

echo.
echo ============================================
echo BUILD COMPLETE!
echo ============================================
echo.
echo Output files:
echo   - output\ELBIX_x64.dll (64-bit Windows)
echo   - output\ELBIX_x86.dll (32-bit Windows)
echo.
echo These DLLs use static runtime (/MT) and should work on any Windows 7+ system
echo without requiring Visual C++ Redistributable.
echo.
echo For macOS build, use build_mac.sh on a Mac machine.
echo ============================================

pause
