# loom2d — test runner
# Usage: .\run_tests.ps1
# Requires the project to already be built with .\build.ps1

$ErrorActionPreference = "Stop"

$BUILD_DIR = "build"
$env:SDL_VIDEODRIVER = "offscreen"
$env:SDL_AUDIODRIVER = "dummy"

Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  loom2d Test Suite" -ForegroundColor Cyan
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# ── C++ tests (Google Test via CTest) ─────────────────────────────────────────
Write-Host "[1/2] C++ unit tests (Google Test)..." -ForegroundColor Yellow

if (-not (Test-Path "$BUILD_DIR\CTestTestfile.cmake")) {
    Write-Host "  Build directory not found. Run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

$ctest_result = & ctest --test-dir $BUILD_DIR -C Debug --output-on-failure 2>&1
$ctest_exit = $LASTEXITCODE
Write-Host $ctest_result

if ($ctest_exit -ne 0) {
    Write-Host ""
    Write-Host "  C++ tests FAILED" -ForegroundColor Red
} else {
    Write-Host "  C++ tests PASSED" -ForegroundColor Green
}

# ── Python tests (pytest) ─────────────────────────────────────────────────────
Write-Host ""
Write-Host "[2/2] Python tests (pytest)..." -ForegroundColor Yellow

$pytest_result = python -m pytest tests/python/ -v --tb=short 2>&1
$pytest_exit = $LASTEXITCODE
Write-Host $pytest_result

if ($pytest_exit -ne 0) {
    Write-Host ""
    Write-Host "  Python tests FAILED" -ForegroundColor Red
} else {
    Write-Host "  Python tests PASSED" -ForegroundColor Green
}

# ── Summary ───────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
if ($ctest_exit -eq 0 -and $pytest_exit -eq 0) {
    Write-Host "  ALL TESTS PASSED" -ForegroundColor Green
    exit 0
} else {
    Write-Host "  SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
