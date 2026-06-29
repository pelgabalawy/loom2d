<h1 align="center">loom2d</h1>

<p align="center">
  <b>Write your 2D game in Python. Ship it everywhere.</b><br>
  A cross-platform 2D game engine with a C++17 core and a clean Python API.
</p>

<p align="center">
  <a href="#status">Status</a> ·
  <a href="#quick-start">Quick Start</a> ·
  <a href="#a-tiny-game">Example</a> ·
  <a href="#architecture">Architecture</a> ·
  <a href="#roadmap">Roadmap</a> ·
  <a href="#contributing">Contributing</a>
</p>

---

## Why loom2d?

Python is a wonderful language to write games in — but shipping a Python game to
Windows, macOS, Linux, Android, and iOS is painful. loom2d solves that by putting
all the performance-critical and platform-specific work in a **C++17 engine** and
exposing it to Python through [pybind11](https://github.com/pybind/pybind11).

You write **pure Python**. The engine handles rendering, input, physics, audio,
and assets in native code that compiles for every target platform.

```
   Your game (Python)  ──▶  loom2d Python API  ──▶  C++ engine  ──▶  every platform
```

## Features

- 🎮 **Pure-Python game code** — subclass `loom.Game`, implement `on_update` / `on_draw`
- 🧱 **Scene graph** — nodes with parent/child transforms, sprites, animations
- 🎨 **GPU rendering** — sokol_gfx sprite batcher, tilemaps with culling, TTF text
- 🕹️ **Input** — keyboard, mouse + wheel, **gamepad** (hot-plug, rumble), **touch**, **text input**
- 📐 **Responsive** — logical resolution + letterbox/stretch/expand scale modes, HiDPI
- ⚙️ **Physics** — [Box2D v3](https://github.com/erincatto/box2d) with a pixel-space API + collision events
- 🔊 **Audio** — SFX + streaming music via [miniaudio](https://github.com/mackron/miniaudio)
- 🖼️ **Assets** — ref-counted texture cache (PNG/JPG via stb_image)
- 🧪 **Tested** — 172 C++ (GoogleTest) + 147 Python (pytest) unit tests
- 📦 **Zero manual deps** — CMake fetches and builds everything for you

## Status

| Platform | Status |
|---|---|
| Windows (x64) | ✅ Builds, all tests pass, examples run |
| macOS | 🚧 Build scripts ready — verification pending |
| Linux | 🚧 Build scripts ready — verification pending |
| Android / iOS | ⏳ Planned |

> **Working today on desktop.** GPU sprite batching (sokol_gfx), tilemaps, text
> rendering, audio, a Box2D-v3 physics engine with collision events, responsive
> resolution scaling, and full input breadth (gamepad, touch, mouse-wheel, text
> input) are all in and tested. A UI toolkit, core game systems (scenes/timers/
> tweens/save-load), a LÖVE-style shader pipeline, and mobile targets are next —
> see the [Roadmap](#roadmap).

## 📖 Documentation

Full guides and API reference: **<https://pelgabalawy.github.io/loom2d/>**

New here? Start with the [Quick Start](https://pelgabalawy.github.io/loom2d/getting-started/quickstart/)
— a controllable sprite in about five minutes, in pure Python.

## Install

Prebuilt, self-contained wheels for **Windows, macOS, and Linux** (CPython
3.11–3.13) are attached to each
[GitHub Release](https://github.com/pelgabalawy/loom2d/releases). Download the
wheel for your platform + Python version and:

```bash
pip install loom2d-<version>-<...>.whl
```

The SDL3 shared library is bundled *inside* the wheel, so there are no system
dependencies to install. To build a wheel yourself instead:

```bash
pip install build
python -m build --wheel        # → dist/loom2d-*.whl
```

## Quick Start (build from source)

You'll need **CMake ≥ 3.21**, a **C++17 compiler**, and **Python ≥ 3.11**.
A first build compiles SDL3 from source, so it takes a few minutes.

**Windows (PowerShell):**
```powershell
.\build.ps1 Debug
.\run_tests.ps1
python examples\hello_world\main.py
```

**macOS / Linux (bash):**
```bash
chmod +x build.sh run_tests.sh
./build.sh Debug
./run_tests.sh
python3 examples/hello_world/main.py
```

Full details — prerequisites, how the SDL library is bundled, troubleshooting —
are in [BUILDING.md](BUILDING.md).

## A tiny game

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color(0.1, 0.1, 0.15)
        print("Hello from loom2d!")

    def on_update(self, dt):
        if loom.Input.key_pressed(loom.Key.Space):
            print("Jump!")
        if loom.Input.key_down(loom.Key.Escape):
            pass  # handled by the engine; window closes on Escape

loom.run(MyGame(), title="My Game", width=800, height=600)
```

See [`examples/`](examples/) for more:

| Example | Shows off |
|---|---|
| [`hello_world`](examples/hello_world) | Minimal window + game loop |
| [`flappy`](examples/flappy) | Component-based architecture, input, collision, scoring |
| [`platformer`](examples/platformer) | Box2D physics, gravity, jumping, collision events |
| [`coin_quest`](examples/coin_quest) | Tilemaps, grid collision, camera follow, text HUD |
| [`responsive`](examples/responsive) | Logical resolution + live scale-mode switching |
| [`input_demo`](examples/input_demo) | Gamepad, touch, mouse-wheel & text input |

## Architecture

```
  Game code (Python)
        │  import loom2d
  Python API  (python/loom2d/)
        │  pybind11
  Native module  (loom2d_native)
        │
  C++ engine core  (src/)
   scene · graphics · input · physics · audio · assets
        │
  SDL3 · Box2D · miniaudio · stb     ← cross-platform foundation
```

A deeper diagram and layer-by-layer breakdown live in
[ARCHITECTURE.md](ARCHITECTURE.md).

## Roadmap

**Shipped — the rendering/gameplay foundation:**

- ✅ **GPU SpriteBatcher** (sokol_gfx) — thousands of sprites at 60fps
- ✅ **Tilemap system** — `.tmx` loader, grid collision, viewport culling, layers
- ✅ **Text rendering** — stb_truetype font atlas, `TextNode`
- ✅ **Physics collision events** — contacts, sensors, raycasts (Box2D v3)
- ✅ **Responsive scaling** — logical resolution + Fit/Stretch/Expand/PixelPerfect
- ✅ **Input breadth** — gamepad (hot-plug, rumble), touch, mouse-wheel, text input

**Next up — toward a polished, shippable game:**

1. **UI toolkit** — panels, buttons, labels, inventory grids, hit-testing
2. **Core game systems** — scene management/transitions, timers, tweens, save/load
3. **LÖVE-style rendering** — custom shaders, blend modes, render-to-texture canvases
4. **Effects** — particles + lighting
5. **Desktop standalone packaging**, then **Android → iOS**

Each platform has its own build-→test-→fix verification pass.

## Contributing

Contributions are very welcome — this project is meant to grow with its community! 🌱

- **All changes land via pull request.** The `main` branch is protected and
  requires maintainer review/approval before merging — please open a PR rather
  than pushing directly.
- Fork the repo, create a branch, and make sure `run_tests` is green before
  opening your PR.
- New features should come with tests (C++ in `tests/cpp/`, Python in `tests/python/`).
- For larger changes, open an issue first to discuss the approach.

## License

[MIT](LICENSE) © Peter Elgabalawy

Bundled dependencies retain their own licenses (SDL3: Zlib · Box2D: MIT ·
miniaudio & stb: public domain · pybind11: BSD).
