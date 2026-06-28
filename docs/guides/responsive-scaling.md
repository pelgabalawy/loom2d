# Responsive Scaling

A loom2d game is authored against a fixed **logical (design) resolution** — say
480×270 — and the engine maps that onto whatever window or screen it's actually
running on: a laptop, a 4K monitor, a HiDPI Retina display, or a phone. You design
once; the engine handles the rest.

```python
class MyGame(loom.Game):
    def on_start(self):
        self.logical_width  = 480   # design resolution
        self.logical_height = 270
        self.scale_mode = loom.ScaleMode.Fit   # the default
```

If you don't set a logical resolution, it defaults to the initial window size and the
window simply fills the screen (the classic behaviour).

!!! tip "Why this matters"
    Your game's coordinates never change. A sprite at `(240, 135)` is always at the
    centre of a 480×270 design — whether the player runs the game in a tiny window or
    fullscreen on a 4K TV. No manual DPI math, no per-resolution layout code.

## Scale modes

The `scale_mode` decides what happens when the window's aspect ratio doesn't match
your logical resolution. Set `self.scale_mode` to any `loom.ScaleMode` value, even
live during gameplay.

| Mode | Behaviour |
|------|-----------|
| `ScaleMode.Fit` | **(default)** Preserve aspect ratio; add letterbox/pillarbox bars to fill the leftover space. Nothing is distorted or cropped. |
| `ScaleMode.Stretch` | Fill the whole window, distorting the aspect ratio if it differs. |
| `ScaleMode.Expand` | Preserve aspect ratio and fill the window by revealing **more** of the world on the longer axis (no bars). |
| `ScaleMode.PixelPerfect` | Like `Fit`, but only scales by whole-number factors (1×, 2×, 3×…) so pixel art stays crisp. |

```python
self.scale_mode = loom.ScaleMode.Fit          # safe default for most games
self.scale_mode = loom.ScaleMode.PixelPerfect # crisp retro pixel art
self.scale_mode = loom.ScaleMode.Expand       # show more world on wide screens
```

### Choosing a mode

- **`Fit`** is the right default for almost everything — your game looks identical on
  every screen, framed by bars when aspect ratios differ.
- **`Expand`** suits games where seeing a bit more of the world on wide monitors is an
  advantage rather than a problem (top-down, side-scrollers).
- **`PixelPerfect`** is for low-resolution pixel art where non-integer scaling would
  blur or shimmer the pixels.
- **`Stretch`** is rarely what you want — it distorts — but it's there for full
  control.

## Reacting to resize

The window is resizable by default. Override `on_resize` to respond when the drawable
surface changes size (a window resize, or moving between monitors with different DPI):

```python
def on_resize(self, w, h):
    print(f"now {w} x {h} device pixels")
    self.reflow_hud()
```

You can also read the current drawable size at any time:

```python
self.screen_width    # current width in device pixels
self.screen_height   # current height in device pixels
```

These are **device pixels** (the real backing-store size, which on a 2× HiDPI display
is twice the logical window size). Your game's own coordinates remain in logical
units regardless.

## How it works

Internally the engine, every frame, asks the window for its real drawable size and
computes a GPU **viewport rectangle** plus the camera's logical viewport according to
the scale mode. For `Fit` and `PixelPerfect` the viewport is centred and smaller than
the surface (the surround becomes the letterbox bars); for `Expand` the camera's
viewport grows so more world fits. The camera projection always works in logical
units, so all your positions, the [camera](camera.md), and
`screen_to_world` stay consistent.

## Try it

The [`responsive`](../examples.md) example draws corner + centre markers against a
480×270 design and lets you switch modes live (Q/W/E/R) while resizing the window:

```bash
python examples/responsive/main.py
```

## See also

- [Camera](camera.md) — logical units, zoom, and screen↔world conversion
- [The Game Loop](../getting-started/game-loop.md) — where `on_resize` fits in
