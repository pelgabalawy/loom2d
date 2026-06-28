# Physics Events

Beyond simulating bodies, the [`PhysicsWorld`](physics.md) tells you **when things
touch**: solid collisions, sensor overlaps (triggers), and ray hits. Combined with
**body tags**, this is how you build coin pickups, "is the player on the ground?"
checks, damage zones, line-of-sight tests, and more.

## Body tags

Give bodies a `tag` so your event handlers can tell who is who:

```python
player.tag = "player"
ground.tag = "ground"
coin.tag   = "coin"
```

Every event hands you the `PhysicsBody` objects involved, and `body.tag` identifies
them.

## Contact events (solid collisions)

Two ways to consume events — pick whichever fits your code:

### 1. Callbacks

Assign a function and it fires during `step()` as collisions begin and end:

```python
def on_start(self):
    self.physics.on_contact_begin = self.handle_contact

def handle_contact(self, a, b):
    tags = {a.tag, b.tag}
    if tags == {"player", "ground"}:
        self.on_ground = True
```

| Callback | Fires when |
|----------|-----------|
| `on_contact_begin` | two solid shapes start touching → `fn(body_a, body_b)` |
| `on_contact_end` | they stop touching → `fn(body_a, body_b)` |
| `on_sensor_begin` | a body enters a sensor → `fn(sensor, visitor)` |
| `on_sensor_end` | a body leaves a sensor → `fn(sensor, visitor)` |

### 2. Drainable lists

If you prefer to poll, read the lists of events from the most recent step. They're
refreshed every `step()`:

```python
def on_update(self, dt):
    for pair in self.physics.contact_begins:   # list of ContactPair
        print(pair.body_a.tag, "hit", pair.body_b.tag)
```

| Property | Contains |
|----------|----------|
| `contact_begins` / `contact_ends` | `ContactPair` — `.body_a`, `.body_b` |
| `sensor_begins` / `sensor_ends` | `SensorPair` — `.sensor`, `.visitor` |

## Sensors (triggers)

A **sensor** detects overlap but never blocks movement — perfect for coins,
checkpoints, and damage zones. Make any collider a sensor with `is_sensor=True`:

```python
coin = self.physics.create_body(loom.BodyType.Static, loom.Vec2(x, y))
coin.add_box(12, 12, is_sensor=True)
coin.tag = "coin"

def on_sensor_begin(sensor, visitor):
    if sensor.tag == "coin" and visitor.tag == "player":
        self.score += 1
        self.physics.destroy_body(sensor)   # safe inside a callback

self.physics.on_sensor_begin = on_sensor_begin
```

!!! tip "Ground checks done right"
    Track how many solid surfaces the player touches by counting in
    `on_contact_begin` / `on_contact_end`. The player can jump only while that count is
    above zero — no more mid-air double jumps. The
    [`platformer`](../examples.md) example does exactly this.

## Raycasting

Cast a ray from one point to another and get the **nearest** hit. Returns a
`RaycastHit` you can test directly with `if`:

```python
hit = self.physics.raycast(loom.Vec2(x1, y1), loom.Vec2(x2, y2))
if hit:
    print("hit", hit.body.tag, "at", hit.point, "normal", hit.normal)
```

| `RaycastHit` field | Meaning |
|--------------------|---------|
| `hit` | `True` if the ray struck a body (also the truth value of the object) |
| `body` | the `PhysicsBody` that was hit (`None` on a miss) |
| `point` | world-space hit position (`Vec2`, pixels) |
| `normal` | surface normal at the hit (unit `Vec2`) |
| `fraction` | how far along the ray the hit occurred, `0`–`1` |

Raycasts are great for line-of-sight ("can the enemy see the player?"), ground probes,
laser sights, and bullet hit-scanning.

## A complete mini-example

This headless snippet (no window needed — physics runs without a GPU) shows all three
features. It mirrors the bundled
[`physics_events`](../examples.md) example:

```python
import loom2d as loom

world = loom.PhysicsWorld(gravity_x=0, gravity_y=980)

ground = world.create_body(loom.BodyType.Static, loom.Vec2(0, 300))
ground.add_box(400, 10); ground.tag = "ground"

zone = world.create_body(loom.BodyType.Static, loom.Vec2(0, 150))
zone.add_box(20, 20, is_sensor=True); zone.tag = "zone"

ball = world.create_body(loom.BodyType.Dynamic, loom.Vec2(0, 0))
ball.add_circle(12); ball.tag = "ball"

world.on_contact_begin = lambda a, b: print("contact:", {a.tag, b.tag})
world.on_sensor_begin  = lambda s, v: print(v.tag, "entered", s.tag)

for _ in range(120):
    world.step(1 / 60)

hit = world.raycast(loom.Vec2(100, -100), loom.Vec2(100, 400))
print("ray hit:", hit.body.tag if hit else None)
```

## See also

- [Physics](physics.md) — creating bodies and colliders
- [Examples](../examples.md) — `physics_events` (console) and `platformer` (playable game)
