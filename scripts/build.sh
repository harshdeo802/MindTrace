#!/usr/bin/env bash
# ── MindTrace Build Script (Linux / macOS) ──────────────────────────────

set -e

BUILD_DIR="build"
BUILD_TYPE="Release"
RUN_TESTS=0
CLEAN=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)   BUILD_TYPE="Debug"; shift ;;
        --clean)   CLEAN=1; shift ;;
        --test)    RUN_TESTS=1; shift ;;
        --help)
            echo "Usage: ./build.sh [options]"
            echo ""
            echo "Options:"
            echo "  --debug    Build in Debug mode (default: Release)"
            echo "  --clean    Clean build directory before building"
            echo "  --test     Run tests after building"
            echo "  --help     Show this help message"
            exit 0
            ;;
        *) shift ;;
    esac
done

echo ""
echo "  ╔══════════════════════════════════════╗"
echo "  ║       MindTrace Build Script         ║"
echo "  ╚══════════════════════════════════════╝"
echo ""
echo "  Build Type: ${BUILD_TYPE}"
echo ""

# Clean if requested
if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
    echo "[*] Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "[1/3] Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Configure
echo "[2/3] Configuring with CMake..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build
echo "[3/3] Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "  [SUCCESS] Build complete!"
echo "  Binary: ${BUILD_DIR}/mindtrace"
echo ""

# Run tests if requested
if [ "$RUN_TESTS" -eq 1 ]; then
    echo "[*] Running tests..."
    cd "$BUILD_DIR"
    ctest --build-config "$BUILD_TYPE" --output-on-failure
    cd ..
fi
