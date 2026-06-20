# Building loom2d

loom2d is a C++17 engine exposed to Python via pybind11. All C++ dependencies
(SDL3, Box2D, miniaudio, stb, GoogleTest, pybind11) are fetched automatically by
CMake — there is **no vcpkg/conan/manual dependency step**.

Outputs land in `python/loom2d/` so the examples and tests can `import loom2d`
directly without `pip install`.

---

## Prerequisites

| Platform | Toolchain | Python |
|---|---|---|
| **Windows** | Visual Studio 2022 or Build Tools 2022 (MSVC), CMake ≥ 3.21 | Python ≥ 3.9 |
| **macOS**   | Xcode command line tools (`xcode-select --install`), CMake ≥ 3.21 | Python ≥ 3.9 (python.org or Homebrew) |
| **Linux**   | `build-essential`, `python3-dev`, CMake ≥ 3.21 (+ `libasound2-dev`/PulseAudio dev headers for audio) | Python ≥ 3.9 |

A first build downloads + compiles SDL3 from source, so expect several minutes.
Installing **Ninja** (`brew install ninja` / `apt install ninja-build`) makes
incremental builds much faster; `build.sh` uses it automatically when present.

---

## Build & test

### Windows (PowerShell)
```powershell
.\build.ps1 Debug      # or: .\build.ps1 Release
.\run_tests.ps1
python examples\hello_world\main.py
```

### macOS / Linux (bash)
```bash
chmod +x build.sh run_tests.sh      # first time only
./build.sh Debug                    # or: ./build.sh Release
./run_tests.sh
python3 examples/hello_world/main.py
```

Both `run_tests.sh`/`run_tests.ps1` run the headless suite
(`SDL_VIDEODRIVER=offscreen`, `SDL_AUDIODRIVER=dummy`): 94 C++ tests (CTest) +
94 Python tests (pytest).

---

## How the SDL shared library is found at runtime

The native module depends on the SDL3 shared library. Rather than requiring it
on a system path, the build places SDL right next to the module in
`python/loom2d/`:

- **Windows** — `SDL3.dll` is copied there; `__init__.py` calls
  `os.add_dll_directory()` on that folder.
- **Linux** — the module is linked with `BUILD_RPATH=$ORIGIN`, so the loader
  finds `libSDL3.so.0` in the module's own directory.
- **macOS** — same idea with `@loader_path` and `libSDL3.0.dylib`.

This keeps the package self-contained and avoids `LD_LIBRARY_PATH` juggling.

---

## Platform status

| Platform | Status |
|---|---|
| Windows (x64) | ✅ Built, all tests pass, examples run |
| macOS         | 🚧 Scripts ready — pending first build + verification |
| Linux         | 🚧 Scripts ready — pending first build + verification |
| Android / iOS | ⏳ Planned (Phases 7–8) |

If a build fails on macOS/Linux, capture the **first** error from the CMake
configure or compile output — that's the one to fix; the rest usually cascade.
