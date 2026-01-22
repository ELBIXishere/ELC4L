#!/bin/bash
# ==============================================================================
# ELC4L - macOS Build Script
# Builds: VST3, AU, Standalone (VST2 if SDK available)
# ==============================================================================

set -e

echo ""
echo "========================================"
echo "  ELC4L macOS Build Script"
echo "========================================"
echo ""

# JUCE 경로 설정
if [ -z "$JUCE_PATH" ]; then
    if [ -d "$HOME/JUCE" ]; then
        JUCE_PATH="$HOME/JUCE"
    elif [ -d "/Applications/JUCE" ]; then
        JUCE_PATH="/Applications/JUCE"
    elif [ -d "$(dirname "$0")/../JUCE" ]; then
        JUCE_PATH="$(dirname "$0")/../JUCE"
    else
        echo "[ERROR] JUCE not found! Set JUCE_PATH environment variable."
        echo "        Example: export JUCE_PATH=~/JUCE"
        exit 1
    fi
fi

echo "[INFO] Using JUCE from: $JUCE_PATH"

# VST2 SDK 경로 (선택적)
CMAKE_VST2=""
if [ -n "$VST2_SDK_PATH" ] && [ -d "$VST2_SDK_PATH" ]; then
    echo "[INFO] VST2 SDK found: $VST2_SDK_PATH"
    CMAKE_VST2="-DVST2_SDK_PATH=$VST2_SDK_PATH"
else
    echo "[INFO] VST2 SDK not set. Building without VST2."
fi

# 빌드 디렉토리 생성
mkdir -p build_mac
cd build_mac

# CMake 설정
echo ""
echo "[INFO] Running CMake configuration..."
cmake -G "Xcode" \
    -DJUCE_PATH="$JUCE_PATH" \
    $CMAKE_VST2 \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    ..

# 빌드
echo ""
echo "[INFO] Building Release configuration..."
cmake --build . --config Release --parallel

echo ""
echo "========================================"
echo "  Build completed successfully!"
echo "========================================"
echo ""
echo "Output files:"
echo "  - VST3: build_mac/ELC4L_artefacts/Release/VST3/ELC4L.vst3"
echo "  - AU: build_mac/ELC4L_artefacts/Release/AU/ELC4L.component"
echo "  - Standalone: build_mac/ELC4L_artefacts/Release/Standalone/ELC4L.app"
if [ -n "$VST2_SDK_PATH" ]; then
    echo "  - VST2: build_mac/ELC4L_artefacts/Release/VST/ELC4L.vst"
fi
echo ""

# AU 유효성 검사 (선택적)
if command -v auval &> /dev/null; then
    echo "[INFO] Running AU validation..."
    echo "(auval -v aufx ELC4 HyAu)"
    echo "Note: Run this manually after installing the plugin."
fi

cd ..
echo "Done!"
