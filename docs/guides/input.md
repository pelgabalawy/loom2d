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
Space  Enter  Escape  Tab  Shift  Ctrl
Backspace  Delete  Home  End
F1  F5  F12
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
| `Input.mouse_released(button)` | `bool` — button released this frame |
| `Input.mouse_wheel()` | `Vec2` — scroll delta **this frame** (`y > 0` = up/away) |

`MouseButton` has `Left`, `Middle`, and `Right`.

```python
# Zoom the camera with the scroll wheel
self.scene.camera.zoom += loom.Input.mouse_wheel().y * 0.1
```

!!! note "Mouse position is in logical units"
    The engine remaps the raw pointer through the active
    [scale mode](responsive-scaling.md) and display DPI every frame, so
    `mouse_position()` is already in your game's logical coordinate space — the
    letterbox bars and HiDPI scaling are accounted for. To turn it into a world
    position (e.g. to place an object where the player clicked), convert with
    [`camera.screen_to_world()`](camera.md#screen-world-conversion).

## Gamepad

Controllers are detected automatically and support hot-plugging — plug one in
mid-game and it just starts working. Pass an `index` (default `0`) to address a
specific pad when more than one is connected. Missing/disconnected pads report
"not pressed" and zero axes, so you never need to guard before reading.

```python
def on_update(self, dt):
    if loom.Input.gamepad_connected(0):
        # Left stick → movement. Axes are -1..1 with a dead-zone already applied.
        self.player.x += loom.Input.gamepad_axis(loom.GamepadAxis.LeftX) * 200 * dt
        self.player.y += loom.Input.gamepad_axis(loom.GamepadAxis.LeftY) * 200 * dt

        if loom.Input.gamepad_pressed(loom.GamepadButton.South):  # A / Cross
            self.jump()
```

| Method | Returns |
|--------|---------|
| `Input.gamepad_count()` | number of connected pads |
| `Input.gamepad_connected(index=0)` | `bool` |
| `Input.gamepad_down(button, index=0)` | button held |
| `Input.gamepad_pressed(button, index=0)` | button pressed this frame |
| `Input.gamepad_released(button, index=0)` | button released this frame |
| `Input.gamepad_axis(axis, index=0)` | `float` — sticks `-1..1`, triggers `0..1` |
| `Input.gamepad_rumble(low, high, duration_ms, index=0)` | haptic pulse (if supported) |

`GamepadButton` values are **layout-neutral** — `South`/`East`/`West`/`North`
are A/B/X/Y on an Xbox pad and ✕/○/□/△ on a PlayStation pad — plus `Back`,
`Guide`, `Start`, `LeftStick`, `RightStick`, `LeftShoulder`, `RightShoulder`, and
the D-pad (`DpadUp`/`DpadDown`/`DpadLeft`/`DpadRight`).

`GamepadAxis` has `LeftX`, `LeftY`, `RightX`, `RightY`, `TriggerLeft`, and
`TriggerRight`.

!!! tip "Dead-zone"
    Stick axes pass through a radial dead-zone (default `0.15`) so a resting stick
    reads exactly `0`. Adjust it with `Input.set_gamepad_deadzone(0.2)`. Triggers
    are one-sided (`0..1`) and are **not** dead-zoned.

## Touch

Finger input arrives as a list of `TouchPoint`s, each with an `id` (stable for
the life of the touch), a `position` (in **logical units**, remapped exactly like
the mouse), and `pressure`. Touch is the input path for the mobile targets, and
works on touchscreens/trackpads on the desktop too.

```python
def on_update(self, dt):
    for t in loom.Input.touches_began():   # fingers that touched down this frame
        self.spawn_at(t.position)

    for t in loom.Input.touches():         # all fingers currently down
        if t.id == self.drag_id:
            self.player.position = t.position

    for t in loom.Input.touches_ended():   # fingers lifted this frame
        ...
```

| Method | Returns |
|--------|---------|
| `Input.touch_count()` | number of active fingers |
| `Input.touches()` | `list[TouchPoint]` — all active |
| `Input.touches_began()` | `list[TouchPoint]` — touched down this frame |
| `Input.touches_ended()` | `list[TouchPoint]` — lifted this frame |

A *tap* is a finger that appears in `touches_began()` and shortly after in
`touches_ended()` near the same spot; a *drag* is a finger that persists across
frames in `touches()` with a moving `position`. Build the gestures your game needs
on top of these primitives.

## Text input

For text fields (chat, naming a save, a console) enable text input so typed
characters — including shifted symbols and IME composition — are delivered as
UTF-8 each frame. It is **off by default** so gameplay keys aren't double-handled.

```python
def on_start(self):
    loom.Input.start_text_input()      # enable typed-character events
    self.buffer = ""

def on_update(self, dt):
    self.buffer += loom.Input.text_input()              # characters typed this frame
    if loom.Input.key_pressed(loom.Key.Backspace) and self.buffer:
        self.buffer = self.buffer[:-1]
```

| Method | Description |
|--------|-------------|
| `Input.start_text_input()` | begin delivering typed characters |
| `Input.stop_text_input()` | stop (back to gameplay-only key handling) |
| `Input.text_input_active()` | `bool` |
| `Input.text_input()` | UTF-8 text typed **this frame** (empty if none) |

Use `text_input()` for the characters and `key_pressed(...)` for editing keys
(`Backspace`, `Delete`, `Left`/`Right`, `Home`, `End`).

## Testing without a window

For headless tests you can inject synthetic events for every device — the same
state the read methods above query — so input-driven logic is unit-testable
without opening a window:

```python
loom.Input.inject_key_down(loom.Key.Right)
loom.Input.inject_mouse_wheel(loom.Vec2(0, 1))
loom.Input.inject_text_input("hi")
loom.Input.inject_gamepad_add(0)
loom.Input.inject_gamepad_button(0, loom.GamepadButton.South, True)
loom.Input.inject_gamepad_axis(0, loom.GamepadAxis.LeftX, 1.0)
loom.Input.inject_touch(1, loom.Vec2(100, 50))
loom.Input.inject_touch_release(1)
```

`Input.new_frame()` clears the per-frame accumulators (wheel delta, typed text,
the began/ended touch lists) the same way the engine does at the top of each frame.

## See also

- [The Game Loop](../getting-started/game-loop.md) — where to read input
- [Camera](camera.md) — converting mouse clicks to world coordinates
- The runnable `examples/input_demo/` exercises every device above.
