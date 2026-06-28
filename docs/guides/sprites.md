# Sprites & Textures

A `SpriteNode` draws a texture in the scene. It is a [`Node`](scene-graph.md), so it
has all the transform properties (position, rotation, scale) plus sprite-specific
ones like tint and flipping.

## Loading a texture

Textures are loaded through the asset manager (`self.assets`), which caches them so
loading the same path twice returns the same GPU texture.

```python
class Game(loom.Game):
    def on_start(self):
        tex = self.assets.texture("player.png")   # PNG or JPG
        self.player = loom.SpriteNode(tex)
        self.scene.add(self.player)
```

Supported formats are whatever `stb_image` reads — most commonly **PNG** (with alpha)
and **JPG**. A `Texture` exposes its dimensions:

```python
tex.width    # pixels
tex.height
tex.path     # the file it was loaded from
```

!!! warning "Load textures in `on_start`"
    Texture loading needs a live GPU context, which exists only after the window
    opens. Load in `on_start()`, never in `__init__()`.

## Sprite properties

```python
sprite = loom.SpriteNode(tex)
sprite.position = loom.Vec2(400, 300)
sprite.origin   = loom.Vec2(0.5, 0.5)        # anchor at center
sprite.scale    = loom.Vec2(2, 2)            # draw at 2×
sprite.rotation = 0.3                        # radians
sprite.tint     = loom.Color(1, 0.5, 0.5, 1) # multiply color (red-ish)
sprite.flip_x   = True                       # mirror horizontally
sprite.flip_y   = False
```

| Property | Type | Description |
|----------|------|-------------|
| `origin` | `Vec2` | normalized anchor: `(0,0)` top-left, `(0.5,0.5)` center |
| `tint` | `Color` | multiplied with the texture; white = unchanged |
| `flip_x` / `flip_y` | `bool` | mirror the sprite on that axis |
| *(inherited)* | | `position`, `x`, `y`, `rotation`, `scale`, `visible` |

### Origin and anchoring

`origin` controls which point of the sprite sits at its `position`:

- `(0, 0)` — top-left corner (the default)
- `(0.5, 0.5)` — center, ideal for things that rotate or that you position by their middle
- `(0.5, 1.0)` — bottom-center, handy for characters standing on the ground

## Tinting and scaling tricks

Because `tint` multiplies the texture and `scale` resizes it, a single 1×1 white
pixel texture becomes a flexible colored rectangle:

```python
# A 1×1 white PNG, tinted and stretched into a colored bar.
bar = loom.SpriteNode(white_pixel_tex)
bar.scale = loom.Vec2(200, 16)            # 200×16 px
bar.tint  = loom.Color(0.2, 0.8, 0.3, 1)  # green
```

Several examples use exactly this to draw platforms and ground without art files.

## Sub-rectangles (sprite sheets)

To draw one cell of a sprite sheet, set a source rectangle in pixels:

```python
sprite.set_source(loom.Rect(32, 0, 32, 32))   # x, y, w, h within the texture
```

For frame-by-frame animation built on this, see **[Animation](animation.md)**.

## Performance: GPU batching

Sprites that share a texture are **batched into a single draw call** by the engine's
sprite batcher (built on sokol_gfx). Thousands of sprites from one texture cost about
the same as one. You can watch the count:

```python
print(self.last_draw_calls)   # draw calls issued last frame
```

The [`stress_test`](../examples.md) example pushes 10,000 sprites and reports the
draw-call count and frame rate.

## See also

- [Assets](assets.md) — how the texture cache works
- [Animation](animation.md) — animate a sprite from a sheet
- [Scene Graph](scene-graph.md) — parenting, transforms, draw order
