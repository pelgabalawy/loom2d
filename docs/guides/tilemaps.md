# Tilemaps

A `Tilemap` draws large grid-based levels efficiently and provides optional grid
collision. It loads [Tiled](https://www.mapeditor.org/) `.tmx`/`.tsx` files, or you
can build maps in code. Because it culls to the camera's view, even a 100×100 map
renders in roughly one draw call per tileset.

`Tilemap` is a [`Node`](scene-graph.md), so it has a transform and lives in the scene
like any other object.

## Loading a Tiled map

```python
class Game(loom.Game):
    def on_start(self):
        self.map = loom.Tilemap.load("levels/world1.tmx")
        self.scene.add(self.map)
```

`Tilemap.load` reads the `.tmx`, its embedded or external `.tsx` tilesets, and the
referenced tileset images (CSV tile encoding). Tile flip flags are honored.

## Building a map in code

```python
tiles = self.assets.texture("tileset.png")

tmap = loom.Tilemap(tile_w=32, tile_h=32, width=40, height=23)
tmap.add_tileset(tiles, tile_w=32, tile_h=32, first_gid=1)

ground = tmap.add_layer("ground")
ground.fill(1)                 # fill every cell with tile gid 1
ground.set(5, 10, 2)           # set one cell (x, y) to gid 2
print(ground.at(5, 10))        # read a cell → 2

self.scene.add(tmap)
```

### Pieces

| Type | What it is |
|------|------------|
| `Tilemap` | the whole map: size, tilesets, layers, collision |
| `TileLayer` | one named grid of tile ids (`name`, `visible`, `opacity`) |
| `Tileset` | maps tile **gids** to sub-rectangles of a texture |

A map can have **multiple layers** (e.g. background, ground, decoration) drawn in
order, and **multiple tilesets** distinguished by their `first_gid`.

## Grid collision

The tilemap carries its own lightweight collision grid, independent of the
[physics engine](physics.md). This is ideal for classic tile-based movement where you
don't need full rigid-body dynamics.

```python
# Mark solid tiles, or derive them from a layer's non-empty cells:
tmap.set_solid(5, 10, True)
tmap.set_collision_from_layer(layer_index=1)   # every non-zero tile becomes solid

# Query during movement:
if tmap.is_solid(tile_x, tile_y):
    ...

# Test a world-space rectangle (e.g. the player's AABB) against solids:
hit = tmap.rect_overlaps_solid(loom.Rect(px, py, pw, ph))
```

### Coordinate conversion

```python
world = tmap.tile_to_world(tx, ty)   # tile index → world position
tile  = tmap.world_to_tile(world)    # world position → tile index (Vec2)
```

Both honor the tilemap's own position and scale (tile rotation is ignored).

## Why it's fast

Each frame the tilemap asks the [camera](camera.md) for its `visible_rect()` and
submits only the tiles inside it. The number of draw calls stays roughly constant
regardless of map size — you can scroll around a huge level for almost free. The
`tiles_drawn` property reports how many tiles were submitted last frame:

```python
print(self.map.tiles_drawn)
```

## See also

- [Camera](camera.md) — scrolling and the `visible_rect()` that drives culling
- [Physics](physics.md) — when you want full rigid-body dynamics instead of grid collision
- [Examples](../examples.md) — `tilemap` (scrolling demo) and `coin_quest` (a real top-down game)
