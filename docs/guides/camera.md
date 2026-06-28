# Camera

Every [`Scene`](scene-graph.md) has a `Camera` that controls what part of the world
is visible. Move it to scroll, change its zoom to get closer, and use its helpers to
convert between screen pixels and world coordinates.

```python
cam = self.scene.camera
```

## Following a target

The most common use is making the camera follow the player. Set its `position` to the
point you want centered:

```python
def on_update(self, dt):
    self.scene.camera.position = self.player.position
```

!!! note "What `position` means"
    The camera's `position` is the **world point shown at the center of the window**.
    At `zoom = 1` and the default camera, world coordinates equal screen pixels, so a
    camera at `(400, 300)` centers an 800×600 window on the origin region you'd
    expect.

## Properties

| Property | Type | Description |
|----------|------|-------------|
| `position` | `Vec2` | world point at the center of the view |
| `zoom` | `float` | `1.0` = 1:1, `2.0` = zoomed in 2×, `0.5` = zoomed out |
| `rotation` | `float` | view rotation in radians |

```python
cam.zoom = 2.0          # zoom in
cam.move(loom.Vec2(10, 0))   # pan right by 10 world units
```

## Screen ↔ world conversion

Input arrives in **screen pixels** (e.g. the mouse position), but your game objects
live in **world coordinates**. Two helpers convert between them:

```python
world = cam.screen_to_world(loom.Input.mouse_position())  # click → world
screen = cam.world_to_screen(self.player.position)        # world → pixels
```

### Pinning a HUD

A classic trick: keep a HUD element glued to the corner of the screen even while the
camera scrolls, by converting a fixed screen offset into world space each frame:

```python
def on_update(self, dt):
    self.scene.camera.position = self.player.position           # camera scrolls
    self.hud.position = self.scene.camera.screen_to_world(loom.Vec2(12, 12))
```

The [`coin_quest`](../examples.md) example uses exactly this for its score readout.

## Viewport culling

`visible_rect()` returns the world-space rectangle the camera currently sees. The
[`Tilemap`](tilemaps.md) uses it internally to draw only on-screen tiles, which is
why even a 100×100 map renders in about one draw call. You can use it too — for
example to skip updating off-screen entities:

```python
view = self.scene.camera.visible_rect()   # Rect in world space
if not view.contains(enemy.position):
    continue   # don't bother updating enemies you can't see
```

## See also

- [Scene Graph](scene-graph.md) — the tree the camera views
- [Tilemaps](tilemaps.md) — automatic culling via `visible_rect()`
- [Input](input.md) — mouse position is in screen space; convert with `screen_to_world`
