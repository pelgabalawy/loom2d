#!/usr/bin/env bash
# loom2d — test runner (Linux/macOS)
# Usage: ./run_tests.sh
# Requires the project to already be built with ./build.sh

set -uo pipefail

BUILD_DIR="build"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# Headless SDL so tests need no display / audio device
export SDL_VIDEODRIVER=offscreen
export SDL_AUDIODRIVER=dummy

# Help the C++ test binary find libSDL3 in the build tree (rpath usually covers
# this, but exporting the loader path makes it robust across distros).
SDL_LIB_DIR="$(dirname "$(find "$BUILD_DIR" \( -name 'libSDL3.so*' -o -name 'libSDL3*.dylib' \) 2>/dev/null | head -n 1)")" || true
if [[ -n "${SDL_LIB_DIR:-}" && "$SDL_LIB_DIR" != "." ]]; then
    export LD_LIBRARY_PATH="${SDL_LIB_DIR}:${LD_LIBRARY_PATH:-}"
    export DYLD_LIBRARY_PATH="${SDL_LIB_DIR}:${DYLD_LIBRARY_PATH:-}"
fi

echo ""
echo "=========================================="
echo "  loom2d Test Suite"
echo "=========================================="
echo ""

# ── C++ tests (Google Test via CTest) ─────────────────────────────────────────
echo "[1/2] C++ unit tests (Google Test)..."

if [[ ! -f "$BUILD_DIR/CTestTestfile.cmake" ]]; then
    echo "  Build directory not found. Run ./build.sh first."
    exit 1
fi

# -C Debug matters for multi-config; ignored by single-config generators.
ctest --test-dir "$BUILD_DIR" -C Debug --output-on-failure
ctest_exit=$?

if [[ $ctest_exit -ne 0 ]]; then
    echo ""
    echo "  C++ tests FAILED"
else
    echo "  C++ tests PASSED"
fi

# ── Python tests (pytest) ─────────────────────────────────────────────────────
echo ""
echo "[2/2] Python tests (pytest)..."

PYTHON="${PYTHON:-python3}"
"$PYTHON" -m pytest tests/python/ -v --tb=short
pytest_exit=$?

if [[ $pytest_exit -ne 0 ]]; then
    echo ""
    echo "  Python tests FAILED"
else
    echo "  Python tests PASSED"
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "=========================================="
if [[ $ctest_exit -eq 0 && $pytest_exit -eq 0 ]]; then
    echo "  ALL TESTS PASSED"
    exit 0
else
    echo "  SOME TESTS FAILED"
    exit 1
fi
