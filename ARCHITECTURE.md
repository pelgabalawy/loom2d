# loom2d — Engine Architecture

## How It All Fits Together

```
╔══════════════════════════════════════════════════════════════════════════╗
║                       GAME DEVELOPER WRITES (Python)                    ║
║                                                                          ║
║   import loom2d as loom                                                  ║
║                                                                          ║
║   class MyGame(loom.Game):                                               ║
║       def on_start(self):                                                ║
║           self.player = loom.Sprite("hero.png")                         ║
║           self.add(self.player)                                          ║
║       def on_update(self, dt):                                           ║
║           if loom.key.pressed(loom.Key.RIGHT):                          ║
║               self.player.x += 200 * dt                                 ║
║                                                                          ║
║   loom.run(MyGame())                                                     ║
╚═══════════════════════╦══════════════════════════════════════════════════╝
                         │  import
                         ▼
╔══════════════════════════════════════════════════════════════════════════╗
║               PYTHON PACKAGE  loom2d/  (thin ergonomic wrappers)        ║
║         game.py · sprite.py · scene.py · input.py · audio.py           ║
╚═══════════════════════╦══════════════════════════════════════════════════╝
                         │  pybind11 calls
                         ▼
╔══════════════════════════════════════════════════════════════════════════╗
║         PYBIND11 BINDING MODULE  loom2d_native.pyd / .so               ║
║              src/bindings/module.cpp                                     ║
║       (type conversion, lifetime mgmt, GIL release for C++ loops)       ║
╚═╦════════════╦═══════════╦══════════╦═══════════╦══════════╦════════════╝
  │            │           │          │           │          │
  ▼            ▼           ▼          ▼           ▼          ▼
╔══════╗  ╔═══════╗  ╔════════╗  ╔═══════╗  ╔════════╗  ╔═══════╗
║SCENE ║  ║RENDER ║  ║ ASSETS ║  ║ AUDIO ║  ║PHYSICS ║  ║ INPUT ║
║──────║  ║───────║  ║────────║  ║───────║  ║────────║  ║───────║
║Node  ║  ║Texture║  ║AssetMgr║  ║Sound  ║  ║Body    ║  ║Key    ║
║Sprite║  ║Batcher║  ║HotLoad ║  ║Music  ║  ║Collider║  ║Mouse  ║
║Camera║  ║Shader ║  ║.pak    ║  ║(minia-║  ║World   ║  ║Touch  ║
║Anim  ║  ║(sokol)║  ║(stb)   ║  ║ audio)║  ║(Box2D) ║  ║Pad    ║
╚═══╦══╝  ╚═══╦═══╝  ╚════╦═══╝  ╚═══╦═══╝  ╚════╦═══╝  ╚═══╦═══╝
    └──────────┴───────────┴──────────┴────────────┴───────────┘
                                       │
                                       ▼
╔══════════════════════════════════════════════════════════════════════════╗
║                    PLATFORM ABSTRACTION LAYER                           ║
║                                                                          ║
║   SDL3 ─────── Window, event loop, gamepad, touch, Android/iOS hooks   ║
║   sokol_gfx ── Metal / OpenGL / Vulkan / D3D11  (auto-selected)        ║
║   miniaudio ── Audio output  (zero dependencies, all platforms)         ║
║   Box2D v3 ─── 2D rigid body physics  (C library)                      ║
╚══════════════════════════╦═══════════════════════════════════════════════╝
                            │
        ┌───────────────────┼────────────────────┐
        ▼                   ▼                    ▼
╔══════════════╗   ╔════════════════╗   ╔═══════════════════╗
║   DESKTOP    ║   ║   ANDROID      ║   ║       iOS         ║
║──────────────║   ║────────────────║   ║───────────────────║
║ .dll/.so/    ║   ║ libengine.so   ║   ║ libengine.a       ║
║ .dylib       ║   ║ + CPython .so  ║   ║ + CPython .a      ║
║              ║   ║ + SDL3 Activity║   ║ + SDL3 UIKit      ║
║ Win/Mac/Lin  ║   ║ → .apk         ║   ║ → .ipa            ║
╚══════════════╝   ╚════════════════╝   ╚═══════════════════╝
```

---

## Build & Deploy Pipeline

```
  Your game code
  ──────────────
  my_game/
  ├── main.py          ← 100% Python
  ├── assets/
  │   ├── hero.png
  │   └── music.ogg
  └── loom2d.toml      ← name, bundle ID, version

        │
        ├─ loom2d run          → live dev on desktop (hot-reload assets)
        ├─ loom2d package win  → game.exe  (Windows installer)
        ├─ loom2d package mac  → game.app  (macOS bundle)
        ├─ loom2d package lin  → game.AppImage
        ├─ loom2d package apk  → game.apk  (Android)
        └─ loom2d package ipa  → game.ipa  (iOS / App Store)

  Each output bundle contains:
  ┌──────────────────────────────────────┐
  │  loom2d_native.so  (C++ engine)      │
  │  python3.12        (embedded CPython)│  ← mobile only
  │  *.pyc             (game bytecode)   │
  │  assets.pak        (packed assets)   │
  └──────────────────────────────────────┘
```

---

## Source Directory Layout

```
loom2d/
├── CMakeLists.txt                  ← top-level CMake (FetchContent for all deps)
├── vcpkg.json                      ← optional: vcpkg C++ package manifest
├── pyproject.toml                  ← Python package build (scikit-build-core)
├── ARCHITECTURE.md                 ← this file
│
├── src/
│   ├── platform/                   ← SDL3 window, file I/O, timer
│   │   ├── window.hpp / .cpp
│   │   └── filesystem.hpp / .cpp
│   ├── graphics/                   ← sokol_gfx + sprite pipeline
│   │   ├── renderer.hpp / .cpp     ← sokol init, frame begin/end
│   │   ├── texture.hpp / .cpp      ← stb_image → GPU texture
│   │   ├── sprite_batcher.hpp/.cpp ← instanced draw calls, 1 DC per atlas
│   │   └── camera.hpp / .cpp
│   ├── scene/
│   │   ├── node.hpp / .cpp         ← base node, transform, parent-child
│   │   ├── sprite_node.hpp / .cpp
│   │   ├── scene.hpp / .cpp        ← root, update/draw tree walk
│   │   └── animation.hpp / .cpp    ← frame strips, tweens
│   ├── audio/
│   │   └── audio.hpp / .cpp        ← miniaudio: SFX one-shot + BGM stream
│   ├── physics/
│   │   └── physics.hpp / .cpp      ← Box2D v3: world, body, collider
│   ├── assets/
│   │   ├── asset_manager.hpp/.cpp  ← ref-counted cache, hot-reload
│   │   └── pak.hpp / .cpp          ← read/write .pak zip archives
│   └── bindings/
│       └── module.cpp              ← PYBIND11_MODULE(loom2d_native, m){...}
│
├── python/
│   └── loom2d/
│       ├── __init__.py             ← re-exports all public symbols
│       ├── game.py                 ← Game base class, loom.run()
│       ├── node.py                 ← Node, Sprite, Camera
│       ├── input.py                ← key/mouse/touch helpers
│       ├── audio.py                ← Sound, Music
│       └── cli.py                  ← `loom2d new/run/package` CLI
│
├── third_party/
│   ├── sokol/                      ← sokol_gfx.h + sokol_log.h (headers)
│   ├── stb/                        ← stb_image.h, stb_truetype.h
│   ├── pybind11/                   ← git submodule or FetchContent
│   └── miniaudio/                  ← miniaudio.h (single-header)
│
├── mobile/
│   ├── android/                    ← NDK CMake + SDL3 Java Activity
│   └── ios/                        ← Xcode project template
│
└── examples/
    ├── hello_world/main.py
    ├── platformer/main.py
    └── top_down_rpg/main.py
```

---

## Layer Responsibilities

| Layer | Language | What It Does |
|---|---|---|
| Game code | Python | What the developer writes — all game logic |
| `loom2d/` package | Python | Ergonomic API wrappers, the public interface |
| `loom2d_native` module | C++ + pybind11 | Bridges Python ↔ C++, type conversion |
| Scene / Graphics / Audio / Physics | C++ | All engine logic, zero Python overhead |
| SDL3 + sokol_gfx | C | Window, GPU context, events |
| OS / GPU Driver | Platform | Hardware |

---

## Third-Party Libraries

| Library | Role | License |
|---|---|---|
| SDL3 | Window, input, gamepad, touch, Android/iOS | Zlib |
| sokol_gfx | Metal/OpenGL/Vulkan/D3D11 abstraction | Zlib |
| pybind11 | C++ ↔ Python bindings | BSD |
| Box2D v3 | 2D rigid body physics | MIT |
| miniaudio | Cross-platform audio (zero deps) | Public domain |
| stb_image | PNG/JPG/BMP loading | Public domain |
| stb_truetype | Font rasterization | Public domain |
| glm | Math: vectors, matrices, transforms | MIT |
| scikit-build-core | Python wheel building via CMake | MIT |

---

## Build Phases

| # | Deliverable | Key Tech |
|---|---|---|
| 1 | Window opens, `loom.run(MyGame())` works | SDL3 + pybind11 |
| 2 | Sprites, camera, 1000 sprites @ 60fps | SDL3 Renderer → sokol_gfx |
| 3 | Scene graph, input, fixed-timestep loop | SDL3 events |
| 4 | 2D physics — gravity, collision | Box2D v3 |
| 5 | SFX + streaming BGM | miniaudio |
| 6 | Asset manager, hot-reload, .pak bundles | Custom |
| 7 | Android APK | NDK + python-for-android |
| 8 | iOS IPA | Xcode + CPython static |
| 9 | Text, Tiled tilemaps (stretch) | stb_truetype |
