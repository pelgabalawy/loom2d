# Assets

The `AssetManager` (`self.assets` on your `Game`) is a **ref-counted texture cache**.
Loading the same image twice returns the *same* GPU texture instead of uploading it
again, so you can call `texture()` freely wherever you need it.

## Loading textures

```python
class Game(loom.Game):
    def on_start(self):
        self.player_tex = self.assets.texture("art/player.png")
        self.enemy_tex  = self.assets.texture("art/enemy.png")
        # Calling texture("art/player.png") again returns the cached object.
```

`texture(path)` returns a [`Texture`](sprites.md), which you hand to a `SpriteNode`,
`Tilemap`, or `Tileset`. Supported formats are those `stb_image` reads — most commonly
**PNG** and **JPG**.

```python
tex.width    # pixels
tex.height
tex.path     # the file it loaded from
```

## Why a cache matters

A tilemap might reference one tileset texture across thousands of tiles; dozens of
enemies might share one sprite sheet. The cache guarantees a single GPU upload per
unique file, which also means those sprites
[batch into one draw call](sprites.md#performance-gpu-batching).

## Clearing the cache

When you tear down a level and want to free textures that are no longer referenced:

```python
self.assets.clear()
```

!!! warning "Keep references to what you still use"
    `clear()` releases the cache's hold on textures. A texture stays alive as long as
    something (a `SpriteNode`, a `Tileset`) still references it, thanks to reference
    counting — but don't rely on the cache to keep an unused texture around.

## Loading textures needs a GPU context

Like all GPU resources, textures can only be created once the window is open. Load them
in [`on_start()`](../getting-started/game-loop.md), not in your constructor.

## A note on bundling assets

For distribution, ship your art alongside your game and load it by relative path. The
bundled examples go one step further and **generate their PNGs procedurally** with the
Python standard library, so the repository carries no binary art — see
[`examples/flappy/main.py`](https://github.com/pelgabalawy/loom2d/blob/main/examples/flappy/main.py)
for a tiny self-contained PNG writer.

## See also

- [Sprites & Textures](sprites.md) — drawing the textures you load
- [Tilemaps](tilemaps.md) — tilesets are textures from the same cache
