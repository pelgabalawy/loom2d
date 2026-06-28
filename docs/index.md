# loom2d

**Write your 2D game in Python. Ship it everywhere.**

loom2d is a cross-platform 2D game engine with a **C++17 core** exposed to Python
through [pybind11](https://github.com/pybind/pybind11). You write pure Python; the
engine handles rendering, input, physics, audio, and assets in native code that
compiles for every target platform.

```
   Your game (Python)  ──▶  loom2d Python API  ──▶  C++ engine  ──▶  every platform
```

## Why loom2d?

Python is a wonderful language to write games in — but shipping a Python game to
Windows, macOS, Linux, Android, and iOS is painful. loom2d puts all the
performance-critical and platform-specific work in native code, so your game code
stays 100% Python while still running fast and shipping everywhere.

## Features

- :video_game: **Pure-Python game code** — subclass `loom.Game`, implement `on_update` / `on_draw`
- :bricks: **Scene graph** — nodes with parent/child transforms, sprites, animations
- :framed_picture: **GPU sprite batching** — thousands of sprites in a handful of draw calls (sokol_gfx)
- :joystick: **Input** — keyboard & mouse (touch/gamepad on the roadmap)
- :gear: **Physics** — [Box2D v3](https://github.com/erincatto/box2d) with a pixel-space API and collision events
- :world_map: **Tilemaps** — Tiled `.tmx`/`.tsx` loader with viewport culling and grid collision
- :pencil: **Text** — TrueType fonts baked to a texture atlas (stb_truetype)
- :speaker: **Audio** — SFX + streaming music via [miniaudio](https://github.com/mackron/miniaudio)

## A tiny game

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color.cornflower()
        tex = self.assets.texture("player.png")
        self.player = loom.SpriteNode(tex)
        self.player.position = loom.Vec2(400, 300)
        self.scene.add(self.player)

    def on_update(self, dt):
        if loom.Input.key_down(loom.Key.Right):
            self.player.x += 200 * dt

loom.run(MyGame(), title="My Game", width=800, height=600)
```

## Where to next?

<div class="grid cards" markdown>

- :material-download: **[Installation](getting-started/installation.md)** — install the wheel and run your first window
- :material-rocket-launch: **[Quick Start](getting-started/quickstart.md)** — build a moving sprite from scratch
- :material-book-open-variant: **[Guides](guides/scene-graph.md)** — one page per engine component
- :material-code-tags: **[API Summary](reference/api-summary.md)** — every class and method at a glance

</div>

!!! note "Project status"
    loom2d is in active development. Desktop (Windows/macOS/Linux) is working today
    and available as prebuilt wheels; mobile (Android/iOS) is on the roadmap. See the
    [README](https://github.com/pelgabalawy/loom2d) for the latest platform status.
