# Input

Input in loom2d is a **static class** — call its methods directly, no instance
needed. State is refreshed each frame before `on_update`, so you can query it freely
inside your update loop.

```python
import loom2d as loom

if loom.Input.key_down(loom.Key.Space):
    ...
```

## Keyboard

There are three ways to ask about a key, and the difference matters:

| Method | True when… | Use for |
|--------|-----------|---------|
| `Input.key_down(key)` | the key is **held** | continuous movement |
| `Input.key_pressed(key)` | the key went down **this frame** | jumps, menu actions, toggles |
| `Input.key_released(key)` | the key came up **this frame** | charge-and-release mechanics |

```python
def on_update(self, dt):
    # Held → continuous movement
    if loom.Input.key_down(loom.Key.Right):
        self.player.x += 200 * dt

    # Edge → one action per press (won't repeat while held)
    if loom.Input.key_pressed(loom.Key.Space):
        self.jump()

    if loom.Input.key_pressed(loom.Key.Escape):
        self.running = False
```

!!! tip "`key_down` vs `key_pressed`"
    Use `key_down` for things that should happen every frame the key is held (walking).
    Use `key_pressed` for things that should happen exactly once per press (jumping,
    firing, confirming a menu) — otherwise a held key fires every frame.

### Available keys

The `Key` enum covers letters `A`–`Z`, arrows (`Up`, `Down`, `Left`, `Right`), and
common keys:

```
Space  Enter  Escape  Tab  Shift  Ctrl  F1  F5  F12
```

## Mouse

```python
pos = loom.Input.mouse_position()                 # Vec2, in logical units
if loom.Input.mouse_down(loom.MouseButton.Left):  # held
    ...
if loom.Input.mouse_pressed(loom.MouseButton.Left):  # clicked this frame
    world = self.scene.camera.screen_to_world(pos)
    self.spawn_at(world)
```

| Method | Returns |
|--------|---------|
| `Input.mouse_position()` | `Vec2` — cursor position in **logical units** |
| `Input.mouse_down(button)` | `bool` — button held |
| `Input.mouse_pressed(button)` | `bool` — button clicked this frame |

`MouseButton` has `Left`, `Middle`, and `Right`.

!!! note "Mouse position is in logical units"
    The engine remaps the raw pointer through the active
    [scale mode](responsive-scaling.md) and display DPI every frame, so
    `mouse_position()` is already in your game's logical coordinate space — the
    letterbox bars and HiDPI scaling are accounted for. To turn it into a world
    position (e.g. to place an object where the player clicked), convert with
    [`camera.screen_to_world()`](camera.md#screen-world-conversion).

## Testing without a window

For headless tests you can inject synthetic key events:

```python
loom.Input.inject_key_down(loom.Key.Right)
loom.Input.inject_key_up(loom.Key.Right)
```

These drive the same state that `key_down` / `key_pressed` read, so you can unit-test
input-driven logic without opening a window.

## On the roadmap

Gamepad and touch input are planned (touch is a prerequisite for the mobile targets).
Track progress on the [GitHub repo](https://github.com/pelgabalawy/loom2d).

## See also

- [The Game Loop](../getting-started/game-loop.md) — where to read input
- [Camera](camera.md) — converting mouse clicks to world coordinates
