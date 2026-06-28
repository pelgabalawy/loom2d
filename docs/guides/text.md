# Text & Fonts

loom2d renders text from TrueType/OpenType fonts. A `Font` bakes the glyphs into a
texture atlas once; a `TextNode` then draws strings using that atlas. Because text
reuses the sprite pipeline, a whole string is **one draw call**.

## Loading a font

`Font.load` needs a live GPU context, so call it from `on_start()`. Give it a path
and a pixel height:

```python
class Game(loom.Game):
    def on_start(self):
        self.font = loom.Font.load("fonts/Roboto.ttf", pixel_height=28)
```

It bakes ASCII glyphs (32–126) into one atlas. A `Font` exposes some metrics:

```python
self.font.pixel_height   # the size it was baked at
self.font.line_height    # vertical distance between baselines
self.font.ascent         # pixels above the baseline
```

!!! tip "Finding a system font"
    For quick demos you can point at a font that ships with the OS, falling back across
    platforms:
    ```python
    import os
    for p in (r"C:\Windows\Fonts\arial.ttf",
              "/System/Library/Fonts/Supplemental/Arial.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
        if os.path.exists(p):
            font = loom.Font.load(p, 28)
            break
    ```

## Drawing text

A `TextNode` is a [`Node`](scene-graph.md), so it has a position, rotation, and scale
like any other object. Add it to the scene:

```python
self.label = loom.TextNode(self.font, "Score: 0")
self.label.position = loom.Vec2(20, 20)
self.label.color = loom.Color(1, 0.95, 0.6, 1)
self.scene.add(self.label)
```

Update the text any time — it re-lays out automatically:

```python
def on_update(self, dt):
    self.label.text = f"Score: {self.score}"
```

## Properties

| Property | Type | Description |
|----------|------|-------------|
| `text` | `str` | the string to draw (re-lays out on change) |
| `color` | `Color` | text color (tints the glyphs) |
| `origin` | `Vec2` | normalized anchor for the whole block, like a sprite |
| `align` | `TextAlign` | `Left`, `Center`, or `Right` |
| `max_width` | `float` | wrap width in pixels; `0` disables wrapping |
| `size` | `Vec2` | (read-only) measured block size |

```python
self.label.align = loom.TextAlign.Center
self.label.max_width = 300        # wrap long lines at 300px
self.label.origin = loom.Vec2(0.5, 0.5)   # center the block on its position
```

## Word wrapping and measuring

To wrap text, set `max_width`. To measure without drawing (e.g. to size a dialog box),
ask the font:

```python
w, h = self.font.measure("Hello, world", max_width=200)
```

## Building a HUD

A score readout that stays pinned to the screen corner even as the camera scrolls —
convert a fixed screen offset into world space each frame:

```python
def on_update(self, dt):
    self.scene.camera.position = self.player.position
    self.hud.position = self.scene.camera.screen_to_world(loom.Vec2(12, 12))
    self.hud.text = f"coins {self.score}/{self.total}"
```

See the [Camera](camera.md#pinning-a-hud) guide for more on screen-space pinning.

## See also

- [Scene Graph](scene-graph.md) — `TextNode` transforms and draw order
- [Camera](camera.md) — pinning a HUD to the view
- [Examples](../examples.md) — `text_demo` (alignment/wrapping) and `coin_quest` (a live HUD)
