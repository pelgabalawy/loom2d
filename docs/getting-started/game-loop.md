# The Game Loop

Everything in loom2d hangs off a single class: `loom.Game`. You subclass it,
override the hooks you need, and pass an instance to `loom.run()`.

## Lifecycle hooks

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        # Called once, after the window and GPU are ready.
        # Load textures, build your scene, create physics bodies here.
        ...

    def on_update(self, dt):
        # Called once per frame. `dt` = seconds since the last frame.
        # Put game logic, input handling, and movement here.
        ...

    def on_draw(self):
        # Called once per frame, after on_update. The scene is drawn
        # automatically; override this only for custom immediate drawing.
        ...

    def on_stop(self):
        # Called once, just before the window closes. Save state here.
        ...

loom.run(MyGame(), title="My Game", width=800, height=600)
```

| Hook | When | Typical use |
|------|------|-------------|
| `on_start()` | once, after GPU is ready | load assets, build the scene, create bodies |
| `on_update(dt)` | every frame | input, game logic, movement |
| `on_draw()` | every frame, after update | custom drawing (optional) |
| `on_stop()` | once, before exit | save game, cleanup |

!!! warning "Load assets in `on_start`, not `__init__`"
    Textures and fonts need a live GPU context, which only exists once the window is
    open. `on_start()` is the first place that is guaranteed. Doing GPU work in your
    constructor will fail.

## What `loom.run()` does each frame

```
poll input  →  step physics  →  on_update(dt)  →  update scene  →  on_draw()  →  render
```

Two of those steps are automatic and can be toggled:

```python
self.auto_physics = True   # step self.physics every frame (default True)
self.auto_scene   = True   # update + draw self.scene every frame (default True)
```

Turn `auto_physics` off if you want to step the world manually (e.g. a fixed
timestep), or `auto_scene` off if you manage drawing yourself.

## Built-in members

A `Game` gives you the core subsystems ready to use — no setup required:

| Member | Type | What it is |
|--------|------|------------|
| `self.scene` | [`Scene`](../guides/scene-graph.md) | the node tree that gets drawn each frame |
| `self.physics` | [`PhysicsWorld`](../guides/physics.md) | the Box2D world (stepped automatically) |
| `self.audio` | [`AudioEngine`](../guides/audio.md) | sound effects and music |
| `self.assets` | [`AssetManager`](../guides/assets.md) | ref-counted texture cache |
| `self.clear_color` | `Color` | background color, cleared every frame |
| `self.running` | `bool` | set to `False` to exit the loop |
| `self.last_draw_calls` | `int` | draw calls issued last frame (diagnostics) |

## Exiting

Set `self.running = False` to end the loop cleanly (then `on_stop()` runs). A common
pattern is quitting on Escape:

```python
def on_update(self, dt):
    if loom.Input.key_pressed(loom.Key.Escape):
        self.running = False
```

## `loom.run()` parameters

```python
loom.run(game, title="loom2d", width=800, height=600)
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `game` | — | your `Game` instance |
| `title` | `"loom2d"` | window title |
| `width` | `800` | window width in pixels |
| `height` | `600` | window height in pixels |

The call **blocks until the window closes**.

!!! tip "Self-terminating runs for testing"
    Several examples accept `--frames N` and set `self.running = False` after `N`
    frames. This is handy for smoke-testing a build in CI or capturing a screenshot
    without a human closing the window.
