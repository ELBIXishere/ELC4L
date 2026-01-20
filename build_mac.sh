#!/bin/bash
echo "============================================"
echo "ELBIX VST2 Plugin - macOS Build"
echo "============================================"
echo ""

# Check for Xcode Command Line Tools
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] CMake not found! Install with: brew install cmake"
    exit 1
fi

if ! command -v clang++ &> /dev/null; then
    echo "[ERROR] Xcode Command Line Tools not found!"
    echo "Install with: xcode-select --install"
    exit 1
fi

# Build directory
BUILD_DIR="build_mac"
OUTPUT_DIR="output"

echo "[1/3] Cleaning previous build..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[2/3] Configuring CMake..."
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
      ..

if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed!"
    cd ..
    exit 1
fi

echo "[3/3] Building..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed!"
    cd ..
    exit 1
fi

cd ..

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Copy VST bundle
cp -R "$BUILD_DIR/bin/HyeokStreamMaster.vst" "$OUTPUT_DIR/ELBIX.vst"

echo ""
echo "============================================"
echo "BUILD COMPLETE!"
echo "============================================"
echo ""
echo "Output: $OUTPUT_DIR/ELBIX.vst"
echo ""
echo "Installation:"
echo "  User:   ~/Library/Audio/Plug-Ins/VST/"
echo "  System: /Library/Audio/Plug-Ins/VST/"
echo ""
echo "Universal Binary: Intel (x86_64) + Apple Silicon (arm64)"
echo "Minimum macOS: 10.13 (High Sierra)"
echo "============================================"
