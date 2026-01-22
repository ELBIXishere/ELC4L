@echo off
REM ==============================================================================
REM ELC4L - Windows Build All Script
REM Builds: VST2 (x86/x64), VST3 (x64)
REM ==============================================================================

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   ELC4L Build All Script
echo ========================================
echo.

set JUCE_PATH=C:\JUCE
set PROJECT_DIR=%~dp0
set OUTPUT_DIR=%PROJECT_DIR%..\output\JUCE_builds

REM 출력 디렉토리 생성
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [INFO] JUCE Path: %JUCE_PATH%
echo [INFO] Project Dir: %PROJECT_DIR%
echo [INFO] Output Dir: %OUTPUT_DIR%
echo.

REM ==============================================================================
REM VST3 x64 빌드
REM ==============================================================================
echo.
echo ========================================
echo   Building VST3 (x64)
echo ========================================
echo.

if exist "%PROJECT_DIR%build_vst3_x64" rmdir /s /q "%PROJECT_DIR%build_vst3_x64"
mkdir "%PROJECT_DIR%build_vst3_x64"
cd "%PROJECT_DIR%build_vst3_x64"

cmake -G "Visual Studio 17 2022" -A x64 ^
    -DJUCE_PATH="%JUCE_PATH%" ^
    -DBUILD_VST3_ONLY=ON ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 (
    echo [ERROR] CMake configuration failed for VST3 x64!
    goto :error
)

cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed for VST3 x64!
    goto :error
)

REM VST3 복사
echo [INFO] Copying VST3 to output...
if not exist "%OUTPUT_DIR%\VST3" mkdir "%OUTPUT_DIR%\VST3"
xcopy /E /Y "%PROJECT_DIR%build_vst3_x64\ELC4L_VST3_artefacts\Release\VST3\*" "%OUTPUT_DIR%\VST3\"

echo [SUCCESS] VST3 x64 build complete!

REM ==============================================================================
REM VST2 x64 빌드
REM ==============================================================================
echo.
echo ========================================
echo   Building VST2 (x64)
echo ========================================
echo.

if exist "%PROJECT_DIR%build_vst2_x64" rmdir /s /q "%PROJECT_DIR%build_vst2_x64"
mkdir "%PROJECT_DIR%build_vst2_x64"
cd "%PROJECT_DIR%build_vst2_x64"

cmake -G "Visual Studio 17 2022" -A x64 ^
    -DJUCE_PATH="%JUCE_PATH%" ^
    -DBUILD_VST2_ONLY=ON ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 (
    echo [ERROR] CMake configuration failed for VST2 x64!
    goto :error
)

cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed for VST2 x64!
    goto :error
)

REM VST2 x64 복사
echo [INFO] Copying VST2 x64 to output...
if not exist "%OUTPUT_DIR%\VST2" mkdir "%OUTPUT_DIR%\VST2"
copy /Y "%PROJECT_DIR%build_vst2_x64\ELC4L_VST2_artefacts\Release\VST\*.dll" "%OUTPUT_DIR%\VST2\ELC4L_VST2_x64.dll"

echo [SUCCESS] VST2 x64 build complete!

REM ==============================================================================
REM VST2 x86 빌드
REM ==============================================================================
echo.
echo ========================================
echo   Building VST2 (x86)
echo ========================================
echo.

if exist "%PROJECT_DIR%build_vst2_x86" rmdir /s /q "%PROJECT_DIR%build_vst2_x86"
mkdir "%PROJECT_DIR%build_vst2_x86"
cd "%PROJECT_DIR%build_vst2_x86"

cmake -G "Visual Studio 17 2022" -A Win32 ^
    -DJUCE_PATH="%JUCE_PATH%" ^
    -DBUILD_VST2_ONLY=ON ^
    -DCMAKE_BUILD_TYPE=Release ^
    ..

if errorlevel 1 (
    echo [ERROR] CMake configuration failed for VST2 x86!
    goto :error
)

cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed for VST2 x86!
    goto :error
)

REM VST2 x86 복사
echo [INFO] Copying VST2 x86 to output...
copy /Y "%PROJECT_DIR%build_vst2_x86\ELC4L_VST2_artefacts\Release\VST\*.dll" "%OUTPUT_DIR%\VST2\ELC4L_VST2_x86.dll"

echo [SUCCESS] VST2 x86 build complete!

REM ==============================================================================
REM 완료
REM ==============================================================================
echo.
echo ========================================
echo   All builds completed successfully!
echo ========================================
echo.
echo Output files:
echo   - %OUTPUT_DIR%\VST3\ELC4L_VST3.vst3
echo   - %OUTPUT_DIR%\VST2\ELC4L_VST2_x64.dll
echo   - %OUTPUT_DIR%\VST2\ELC4L_VST2_x86.dll
echo.

cd "%PROJECT_DIR%"
goto :end

:error
echo.
echo [ERROR] Build failed!
cd "%PROJECT_DIR%"
exit /b 1

:end
pause
