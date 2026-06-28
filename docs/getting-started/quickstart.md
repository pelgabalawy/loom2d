# Quick Start

This page takes you from an empty file to a controllable sprite in about five
minutes. Every line here is plain Python — the same code runs unchanged on
Windows, macOS, and Linux.

## 1. A window

Every loom2d game is a subclass of `loom.Game`. Override the lifecycle hooks you
care about and hand an instance to `loom.run()`.

```python
import loom2d as loom

class Game(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color.cornflower()  # a pleasant blue

loom.run(Game(), title="Hello loom2d", width=800, height=600)
```

Run it:

```bash
python game.py
```

You get a window filled with a cornflower-blue background. Close it (or press the
window's close button) to exit. That is the whole engine bootstrap — no manual
event loop, no GL setup.

## 2. Draw a sprite

Load a texture through the asset manager and add a `SpriteNode` to the scene. The
scene is drawn for you every frame.

```python
import loom2d as loom

class Game(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color.cornflower()
        tex = self.assets.texture("player.png")     # (1)
        self.player = loom.SpriteNode(tex)
        self.player.origin = loom.Vec2(0.5, 0.5)    # (2)
        self.player.position = loom.Vec2(400, 300)  # center of an 800×600 window
        self.scene.add(self.player)

loom.run(Game(), title="Sprite", width=800, height=600)
```

1. `self.assets` is a ref-counted texture cache. Loading the same path twice
   returns the same GPU texture.
2. `origin` is normalized: `(0.5, 0.5)` anchors the sprite by its center, so
   `position` is its middle. `(0, 0)` (the default) anchors the top-left.

!!! tip "No art yet?"
    Every bundled example procedurally generates its own PNGs with nothing but the
    Python standard library — see
    [`examples/flappy/main.py`](https://github.com/pelgabalawy/loom2d/blob/main/examples/flappy/main.py)
    for a ~10-line PNG writer you can copy.

## 3. Move it with input

`on_update(dt)` runs once per frame; `dt` is the seconds elapsed since the last
frame. Multiply movement by `dt` so it is frame-rate independent.

```python
    def on_update(self, dt):
        speed = 250.0  # pixels per second
        if loom.Input.key_down(loom.Key.Left)  or loom.Input.key_down(loom.Key.A):
            self.player.x -= speed * dt
        if loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.D):
            self.player.x += speed * dt
        if loom.Input.key_down(loom.Key.Up)    or loom.Input.key_down(loom.Key.W):
            self.player.y -= speed * dt
        if loom.Input.key_down(loom.Key.Down)  or loom.Input.key_down(loom.Key.S):
            self.player.y += speed * dt
```

!!! note "Y points down"
    Screen coordinates start at the top-left, so **increasing `y` moves down**. This
    matches most 2D engines and image formats.

## Full program

```python
import loom2d as loom

class Game(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color.cornflower()
        tex = self.assets.texture("player.png")
        self.player = loom.SpriteNode(tex)
        self.player.origin = loom.Vec2(0.5, 0.5)
        self.player.position = loom.Vec2(400, 300)
        self.scene.add(self.player)

    def on_update(self, dt):
        speed = 250.0
        if loom.Input.key_down(loom.Key.Left):  self.player.x -= speed * dt
        if loom.Input.key_down(loom.Key.Right): self.player.x += speed * dt
        if loom.Input.key_down(loom.Key.Up):    self.player.y -= speed * dt
        if loom.Input.key_down(loom.Key.Down):  self.player.y += speed * dt
        if loom.Input.key_pressed(loom.Key.Escape):
            self.running = False

loom.run(Game(), title="Quick Start", width=800, height=600)
```

## Where to next?

- Understand the lifecycle in detail → **[The Game Loop](game-loop.md)**
- Organize many objects → **[Scene Graph](../guides/scene-graph.md)**
- Add gravity and collisions → **[Physics](../guides/physics.md)**
- Browse complete games → **[Examples](../examples.md)**
