# Physics

loom2d includes a 2D physics engine powered by [Box2D v3](https://box2d.org/). Your
`Game` already owns a `PhysicsWorld` as `self.physics`, and it is **stepped
automatically** every frame. You create bodies, attach colliders, and read back their
positions to drive your sprites.

!!! info "Everything is in pixels"
    Box2D works in meters; loom2d's API works in **pixels** and converts for you
    (64 pixels = 1 meter internally). You never deal with meters — positions,
    velocities, forces, and sizes are all pixel-space.

## A falling box

```python
class Game(loom.Game):
    def on_start(self):
        # Static ground near the bottom of an 800×600 window
        ground = self.physics.create_body(loom.BodyType.Static, loom.Vec2(400, 580))
        ground.add_box(half_w=400, half_h=20)

        # A dynamic box that will fall onto it
        self.box = self.physics.create_body(loom.BodyType.Dynamic, loom.Vec2(400, 100))
        self.box.add_box(half_w=16, half_h=16)

        self.box_node = loom.SpriteNode(self.assets.texture("box.png"))
        self.box_node.origin = loom.Vec2(0.5, 0.5)
        self.scene.add(self.box_node)

    def on_update(self, dt):
        # Sync the sprite to the physics body each frame
        self.box_node.position = self.box.position
        self.box_node.rotation = self.box.rotation
```

Gravity defaults to `(0, 980)` px/s² — downward, since +Y is down. The world is
stepped for you, so the box just falls.

## Body types

| `BodyType` | Moves? | Affected by forces/gravity? | Use for |
|------------|--------|-----------------------------|---------|
| `Static` | no | no | ground, walls, platforms |
| `Dynamic` | yes | yes | players, crates, projectiles |
| `Kinematic` | yes (by you) | no | moving platforms you drive manually |

```python
body = self.physics.create_body(loom.BodyType.Dynamic, loom.Vec2(x, y))
```

## Colliders

Attach one or more shapes to a body. Sizes are **half-extents** for boxes and a radius
for circles, both in pixels.

```python
body.add_box(half_w=16, half_h=24, density=1.0, friction=0.3, restitution=0.0)
body.add_circle(radius=12, density=1.0, friction=0.3, restitution=0.2)
```

| Parameter | Meaning |
|-----------|---------|
| `density` | mass per area; higher = heavier |
| `friction` | surface friction `[0,1]` |
| `restitution` | bounciness; `0` = no bounce, `1` = perfectly elastic |
| `is_sensor` | if `True`, detects overlap but never blocks — see [Physics Events](physics-events.md) |

## Reading and driving bodies

```python
body.position          # Vec2, pixels (read-only property)
body.rotation          # float, radians
body.linear_velocity   # Vec2, pixels/sec

body.set_position(loom.Vec2(100, 200))
body.set_linear_velocity(loom.Vec2(150, 0))
body.apply_impulse(loom.Vec2(0, -400))   # instantaneous (a jump)
body.apply_force(loom.Vec2(50, 0))       # continuous (wind, thrust)
body.tag = "player"                      # label for collision events
```

!!! tip "Impulse vs force"
    Use `apply_impulse` for an instant change in velocity (a jump, a kick). Use
    `apply_force` for something sustained over time (thrust, wind). Apply forces every
    frame they should act.

## The classic platformer pattern

```python
def on_update(self, dt):
    vx = 0
    if loom.Input.key_down(loom.Key.Left):  vx -= 220
    if loom.Input.key_down(loom.Key.Right): vx += 220
    # Set horizontal velocity directly, keep the body's own vertical velocity
    vy = self.player.linear_velocity.y
    self.player.set_linear_velocity(loom.Vec2(vx, vy))

    # Sync the sprite
    self.player_node.position = self.player.position
```

To make jumping only work while standing on the ground, use **contact events** —
covered in [Physics Events](physics-events.md).

## Stepping manually

By default the world steps automatically. For a fixed timestep or custom control, turn
that off and step yourself:

```python
self.auto_physics = False
# ...
def on_update(self, dt):
    self.physics.step(dt, sub_steps=4)
```

## Cleaning up

```python
self.physics.destroy_body(body)
```

It is safe to call this from inside a collision callback (e.g. destroying a coin when
the player touches it).

## See also

- **[Physics Events](physics-events.md)** — contacts, sensors, raycasts, and body tags
- [Examples](../examples.md) — the `platformer` is a full playable game on the physics engine
