# Math Types

loom2d's three small value types — `Vec2`, `Rect`, and `Color` — show up throughout
the API. They're cheap to create and have convenient helpers.

## Vec2

A 2D vector / point. Used for positions, velocities, sizes, and directions.

```python
v = loom.Vec2(3, 4)
v.x, v.y                 # 3.0, 4.0
v.length()               # 5.0
v.normalized()           # unit vector
v.dot(other)             # dot product
v.distance(other)        # distance to another point
v.lerp(other, 0.5)       # linear interpolation
v.rotated(angle)         # rotate by radians
```

Vectors support the usual operators (`+`, `-`, scalar `*`), so you can write:

```python
self.player.position = self.player.position + velocity * dt
```

### Methods

| Member | Returns | Description |
|--------|---------|-------------|
| `Vec2(x, y)` | `Vec2` | construct |
| `x`, `y` | `float` | components |
| `length()` | `float` | magnitude |
| `length_sq()` | `float` | magnitude squared (cheaper; good for comparisons) |
| `normalized()` | `Vec2` | unit-length copy |
| `dot(o)` | `float` | dot product |
| `distance(o)` | `float` | distance to `o` |
| `lerp(o, t)` | `Vec2` | interpolate toward `o` by `t` in `[0,1]` |
| `rotated(a)` | `Vec2` | rotated by `a` radians |

### Constants

```python
loom.Vec2.zero()   # (0, 0)
loom.Vec2.one()    # (1, 1)
loom.Vec2.up()     # (0, -1)   — up is negative Y
loom.Vec2.down()   # (0,  1)
loom.Vec2.left()   # (-1, 0)
loom.Vec2.right()  # ( 1, 0)
```

## Rect

An axis-aligned rectangle (`x`, `y` is the top-left corner; `w`, `h` the size).

```python
r = loom.Rect(10, 20, 100, 50)   # x, y, w, h
r.left(); r.right(); r.top(); r.bottom()
r.center()                       # Vec2
r.contains(loom.Vec2(40, 30))    # point inside?
r.intersects(other)              # overlaps another rect?
r.intersection(other)            # overlapping Rect
r.expanded(8)                    # grow by 8px on every side
```

`Rect` is used for sprite source rectangles, tilemap queries
([`rect_overlaps_solid`](../guides/tilemaps.md#grid-collision)), and the camera's
[`visible_rect()`](../guides/camera.md#viewport-culling).

## Color

An RGBA color with components in `[0, 1]`.

```python
c = loom.Color(1.0, 0.5, 0.0, 1.0)   # r, g, b, a  (a defaults to 1)
c.r, c.g, c.b, c.a
```

### Named colors

```python
loom.Color.black()        loom.Color.white()
loom.Color.red()          loom.Color.green()       loom.Color.blue()
loom.Color.yellow()       loom.Color.transparent()
loom.Color.cornflower()   # the classic friendly background blue
```

`Color` is used for `clear_color`, sprite `tint`, and text `color`.

## See also

- [Sprites & Textures](../guides/sprites.md) — `tint` and `Rect` source rectangles
- [Scene Graph](../guides/scene-graph.md) — `Vec2` positions and scales
