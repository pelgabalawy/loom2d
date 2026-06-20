#!/usr/bin/env bash
# loom2d — Linux/macOS build helper
# Usage: ./build.sh [Release|Debug]
# Requires: CMake >= 3.21, a C/C++ compiler (clang or gcc), Python >= 3.9
#           (macOS: Xcode command line tools;  Linux: build-essential + python3-dev)

set -euo pipefail

CONFIG="${1:-Debug}"
BUILD_DIR="build"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# ── Pick a fast generator if available ────────────────────────────────────────
if command -v ninja >/dev/null 2>&1; then
    GEN=(-G Ninja)
else
    GEN=()   # default: Unix Makefiles
fi

# Detect platform for messaging / shared-lib glob
case "$(uname -s)" in
    Darwin) PLATFORM="macOS"; LIBGLOB="libSDL3*.dylib" ;;
    Linux)  PLATFORM="Linux"; LIBGLOB="libSDL3.so*"    ;;
    *)      PLATFORM="$(uname -s)"; LIBGLOB="libSDL3*" ;;
esac

echo ""
echo "=========================================="
echo "  loom2d Build  ($CONFIG)  —  $PLATFORM"
echo "=========================================="
echo ""

# ── Configure ─────────────────────────────────────────────────────────────────
echo "[1/3] Configuring with CMake..."
cmake -S . -B "$BUILD_DIR" "${GEN[@]}" \
      -DCMAKE_BUILD_TYPE="$CONFIG" \
      -DLOOM2D_BUILD_TESTS=ON

# ── Build ─────────────────────────────────────────────────────────────────────
echo ""
echo "[2/3] Building..."
# --config is a no-op for single-config generators (Make/Ninja) but harmless,
# and correct for multi-config; pass parallel jobs.
cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel

# ── Copy outputs → python/loom2d/ ─────────────────────────────────────────────
echo ""
echo "[3/3] Copying outputs to python/loom2d/..."
DEST="python/loom2d"
mkdir -p "$DEST"

# Native module (loom2d_native.cpython-*.so / .dylib)
MODULE="$(find "$BUILD_DIR" -name 'loom2d_native*.so' -o -name 'loom2d_native*.dylib' 2>/dev/null \
          | head -n 1 || true)"
if [[ -n "${MODULE:-}" ]]; then
    cp -f "$MODULE" "$DEST/"
    echo "  Copied $(basename "$MODULE")"
else
    echo "  WARNING: loom2d_native module not found in $BUILD_DIR"
fi

# SDL shared library (and any SONAME symlinks) next to the module
found_sdl=0
while IFS= read -r lib; do
    cp -af "$lib" "$DEST/"
    echo "  Copied $(basename "$lib")"
    found_sdl=1
done < <(find "$BUILD_DIR" -name "$LIBGLOB" 2>/dev/null)
if [[ "$found_sdl" -eq 0 ]]; then
    echo "  WARNING: SDL3 shared library ($LIBGLOB) not found in $BUILD_DIR"
fi

echo ""
echo "=========================================="
echo "  Build complete!"
echo ""
echo "  Run the hello world:"
echo "    python3 examples/hello_world/main.py"
echo ""
echo "  Run all tests:"
echo "    ./run_tests.sh"
echo "=========================================="
