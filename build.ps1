# loom2d — Windows build helper
# Usage: .\build.ps1 [Release|Debug]
# Requires: CMake >= 3.21, Visual Studio 2022 (or Build Tools), Python >= 3.9

param([string]$Config = "Debug")

$ErrorActionPreference = "Stop"
$BUILD_DIR = "build"

Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  loom2d Build  ($Config)" -ForegroundColor Cyan
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# ── Configure ─────────────────────────────────────────────────────────────────
Write-Host "[1/3] Configuring with CMake..." -ForegroundColor Yellow
cmake -S . -B $BUILD_DIR -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_BUILD_TYPE=$Config `
      -DLOOM2D_BUILD_TESTS=ON

# ── Build ─────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "[2/3] Building..." -ForegroundColor Yellow
cmake --build $BUILD_DIR --config $Config --parallel

# ── Copy outputs → python/loom2d/ ─────────────────────────────────────────────
Write-Host ""
Write-Host "[3/3] Copying outputs to python/loom2d/..." -ForegroundColor Yellow

$pyd = Get-ChildItem "$BUILD_DIR" -Recurse -Filter "loom2d_native*.pyd" |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($pyd) {
    Copy-Item $pyd.FullName "python\loom2d\" -Force
    Write-Host "  Copied $($pyd.Name)" -ForegroundColor Green
} else {
    Write-Host "  WARNING: loom2d_native.pyd not found" -ForegroundColor Yellow
}

$dll = Get-ChildItem "$BUILD_DIR" -Recurse -Filter "SDL3.dll" |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($dll) {
    Copy-Item $dll.FullName "python\loom2d\" -Force
    Write-Host "  Copied SDL3.dll" -ForegroundColor Green
}

Write-Host ""
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Build complete!" -ForegroundColor Green
Write-Host ""
Write-Host "  Run the hello world:"    -ForegroundColor White
Write-Host "    python examples\hello_world\main.py" -ForegroundColor Yellow
Write-Host ""
Write-Host "  Run all tests:"          -ForegroundColor White
Write-Host "    .\run_tests.ps1"       -ForegroundColor Yellow
Write-Host "══════════════════════════════════════════" -ForegroundColor Cyan
