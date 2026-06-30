# UI Toolkit

loom2d ships a small **screen-space UI layer** for menus, HUDs, buttons and
panels. It lives on `game.ui` (a `UICanvas`) and is drawn **on top of the scene
with its own fixed camera**, so the interface stays put no matter where your
world camera roams or how the window is resized.

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        self.font = loom.Font.load("C:/Windows/Fonts/arial.ttf", 18)

        btn = loom.Button(self.font, "Play")
        btn.anchor = loom.Vec2(0.5, 0.5)   # centre of the screen
        btn.pivot  = loom.Vec2(0.5, 0.5)
        btn.size   = loom.Vec2(160, 48)
        btn.on_clicked = self.start_level
        self.ui.add(btn)

    def start_level(self):
        print("clicked!")
```

The run loop lays out the UI, dispatches the pointer (firing button callbacks),
and draws it every frame — you only build the widget tree.

## Anchored layout

Every widget is positioned **relative to its parent** with four fields:

| Field | Meaning |
|-------|---------|
| `anchor` | a point in the **parent** rect: `(0,0)` = top-left, `(1,1)` = bottom-right |
| `pivot` | the point in the **widget** that lands on the anchor (same `0..1` space) |
| `offset` | a pixel nudge applied after anchoring |
| `size` | the widget's size in logical pixels |

`size` has one twist that makes the common cases easy: a **non-positive
component means "fill the parent dimension plus that value"**. So `size = (0, 0)`
stretches to fill the parent, and `size = (-32, -32)` fills it with a 16px inset
on every edge. Omit `size` entirely and a widget fills its parent.

```python
# Top-left, fixed size (the default anchoring)
w.size = loom.Vec2(120, 40)

# Centred
w.anchor = loom.Vec2(0.5, 0.5); w.pivot = loom.Vec2(0.5, 0.5)

# Pinned to the bottom-right with a 12px inset
w.anchor = loom.Vec2(1, 1); w.pivot = loom.Vec2(1, 1)
w.offset = loom.Vec2(-12, -12)

# A full-width top bar, 44px tall
bar.size = loom.Vec2(0, 44)   # width 0 → fill parent width
```

!!! tip "It's the same model as the scaling layer"
    The UI is laid out against your **logical resolution** (see
    [Responsive Scaling](responsive-scaling.md)), so a HUD designed at 640×360
    lands in the same place on every screen size.

## Widgets

All widgets share the [layout](#anchored-layout) fields plus `name`, `visible`
(drawn?) and `enabled` (receives input?). Add children to any widget with
`add_child`; add top-level widgets to the canvas with `game.ui.add(widget)`.

### Panel

A solid rectangle — the basic container/backdrop. Optionally bordered.

```python
panel = loom.Panel(loom.Color(0, 0, 0, 0.55))   # translucent black
panel.border_color = loom.Color(1, 1, 1, 0.15)
panel.border_width = 1
game.ui.add(panel)
```

### Label

A run of text. Wraps to the widget's width when `size.x > 0`. Align horizontally
with `align` and vertically with `vcenter`.

```python
label = loom.Label(font, "Score: 0")
label.color = loom.Color.white()
label.align = loom.TextAlign.Right
label.vcenter = True
label.text = "Score: 100"     # update any time
```

### Button

A clickable widget with hover / press / disabled background colours and a centred
caption. Assign `on_clicked` (a plain Python callable). Buttons are `focusable`.

```python
btn = loom.Button(font, "Start")
btn.bg         = loom.Color(0.20, 0.22, 0.28, 1)
btn.bg_hover   = loom.Color(0.28, 0.32, 0.40, 1)
btn.bg_pressed = loom.Color(0.14, 0.16, 0.20, 1)
btn.on_clicked = lambda: print("go!")
```

A click fires when the pointer is **pressed and released over the same button** —
press, drag off, and release elsewhere cancels it, just like a native button.
Set `enabled = False` to grey it out and ignore input.

### Image

Draws a texture (optionally a `source` sub-rect) stretched to the widget rect,
with an optional `tint`.

```python
icon = loom.Image(game.assets.texture("icon.png"))
icon.tint = loom.Color(1, 0.85, 0.2, 1)
icon.anchor = loom.Vec2(1, 1); icon.pivot = loom.Vec2(1, 1)
icon.size = loom.Vec2(40, 40)
```

### Grid

A container that arranges its children into a `columns`-wide grid of equal cells
(rows grow as needed), with optional `spacing` between cells. A child with no
explicit size fills its cell; a child with a size is anchored within its cell.

```python
grid = loom.Grid(columns=4, spacing=loom.Vec2(8, 8))
grid.size = loom.Vec2(260, 60)
for name in ("New", "Load", "Options", "Quit"):
    grid.add_child(loom.Button(font, name))
game.ui.add(grid)
```

## Focus & keyboard

Clicking a `focusable` widget focuses it (clicking empty space clears focus). You
can also drive focus from code — handy for keyboard/gamepad menu navigation:

```python
def on_update(self, dt):
    if loom.Input.key_pressed(loom.Key.Tab):
        self.ui.focus_next()          # cycle through focusable widgets
    if loom.Input.key_pressed(loom.Key.Enter) and self.play_button.focused:
        self.start_level()
```

| Call | Effect |
|------|--------|
| `ui.focus(widget)` | focus a specific widget (ignored if not `focusable`) |
| `ui.clear_focus()` | drop focus |
| `ui.focus_next()` | move to the next focusable widget in tree order (wraps) |
| `widget.focused` | read whether a widget currently holds focus |

Each widget also exposes read-only `hovered` and `pressed` flags if you want to
react to pointer state yourself.

## How it fits together

The UI is independent of the world: `game.ui` has its **own camera** locked to
the logical screen, so the scene camera can pan, zoom and rotate while the HUD
stays fixed. Rendering shares the sprite batcher with the scene — the world and
the UI are drawn in a single GPU buffer upload per frame, and `last_draw_calls`
counts both.

Because layout, hit-testing and click dispatch are pure (no GPU), you can unit
-test your menus headlessly:

```python
canvas = loom.UICanvas()
canvas.set_screen(640, 360)
canvas.add(my_button)
canvas.layout()
canvas.update_input(loom.Vec2(320, 180), pressed=True,  down=True,  released=False)
canvas.update_input(loom.Vec2(320, 180), pressed=False, down=False, released=True)
# my_button.on_clicked has now fired
```

See the runnable [`ui_demo`](../examples.md) example for every widget working
together over a scrolling world.
