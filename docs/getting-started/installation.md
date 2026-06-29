# Installation

loom2d ships as a self-contained Python wheel — the C++ engine and its native
dependencies (SDL3, Box2D, etc.) are bundled inside, so there is nothing else to
install.

## Requirements

- **Python 3.11, 3.12, or 3.13**
- **Windows, macOS, or Linux** (64-bit)

## Install from a release wheel

Prebuilt wheels are published with each [GitHub Release](https://github.com/pelgabalawy/loom2d/releases).
Download the wheel matching your platform and Python version, then:

```bash
pip install loom2d-0.3.0-cp311-cp311-win_amd64.whl
```

Verify the install:

```bash
python -c "import loom2d; print('loom2d ready')"
```

!!! tip "Use a virtual environment"
    ```bash
    python -m venv .venv
    # Windows:        .venv\Scripts\activate
    # macOS / Linux:  source .venv/bin/activate
    pip install <the-wheel>.whl
    ```

## Build from source

If there is no wheel for your platform, or you want to hack on the engine, you can
build it yourself. You will need **CMake ≥ 3.21**, a **C++17 compiler**, and Python.

=== "Windows"

    ```powershell
    git clone https://github.com/pelgabalawy/loom2d
    cd loom2d
    .\build.ps1 Release      # configures, builds, copies the .pyd + SDL3.dll
    ```

=== "macOS / Linux"

    ```bash
    git clone https://github.com/pelgabalawy/loom2d
    cd loom2d
    chmod +x build.sh
    ./build.sh Release       # uses Ninja if available
    ```

All C++ dependencies are fetched automatically by CMake (`FetchContent`) — there is
nothing to install by hand. See
[BUILDING.md](https://github.com/pelgabalawy/loom2d/blob/main/BUILDING.md) for the
full per-platform details, including how the SDL3 shared library is bundled.

After a source build, the `loom2d` package lives in `python/loom2d/`. Run an example
to confirm everything works:

```bash
python examples/hello_world/main.py
```

## Next step

Head to the [Quick Start](quickstart.md) to write your first game.
