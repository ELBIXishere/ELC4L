@echo off
REM ==============================================================================
REM ELC4L - Windows Build Script
REM Builds: VST3, Standalone (VST2 if SDK available)
REM ==============================================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   ELC4L Windows Build Script
echo ========================================
echo.

REM JUCE 경로 설정 (필요시 수정)
if not defined JUCE_PATH (
    if exist "C:\JUCE" (
        set JUCE_PATH=C:\JUCE
    ) else if exist "%~dp0..\JUCE" (
        set JUCE_PATH=%~dp0..\JUCE
    ) else (
        echo [ERROR] JUCE not found! Set JUCE_PATH environment variable.
        echo         Example: set JUCE_PATH=C:\JUCE
        exit /b 1
    )
)

echo [INFO] Using JUCE from: %JUCE_PATH%

REM VST2 SDK 경로 (선택적)
if defined VST2_SDK_PATH (
    echo [INFO] VST2 SDK found: %VST2_SDK_PATH%
    set CMAKE_VST2=-DVST2_SDK_PATH="%VST2_SDK_PATH%"
) else (
    echo [INFO] VST2 SDK not set. Building without VST2.
    set CMAKE_VST2=
)

REM 빌드 디렉토리 생성
if not exist "build_win" mkdir build_win
cd build_win

REM CMake 설정
echo.
echo [INFO] Running CMake configuration...
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DJUCE_PATH="%JUCE_PATH%" ^
    %CMAKE_VST2% ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    cd ..
    exit /b 1
)

REM 빌드
echo.
echo [INFO] Building Release configuration...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)

echo.
echo ========================================
echo   Build completed successfully!
echo ========================================
echo.
echo Output files:
echo   - VST3: build_win\ELC4L_artefacts\Release\VST3\ELC4L.vst3
echo   - Standalone: build_win\ELC4L_artefacts\Release\Standalone\ELC4L.exe
if defined VST2_SDK_PATH (
    echo   - VST2: build_win\ELC4L_artefacts\Release\VST\ELC4L.dll
)
echo.

cd ..
pause
