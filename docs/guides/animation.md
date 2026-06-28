# Animation

loom2d animates sprites by flipping through **sub-rectangles of a texture** (a sprite
sheet) over time. You define named `Animation` clips, attach them to a `SpriteNode`,
and play them by name.

## A clip from a sprite strip

The quickest path is `add_strip`, which slices a grid of frames out of a sheet for
you:

```python
import loom2d as loom

run = loom.Animation("run", loop=True)
run.add_strip(
    sheet_w=256, sheet_h=64,    # full sheet size in pixels
    frame_w=64,  frame_h=64,    # one frame
    cols=4, rows=1,             # 4 frames in a row
    frame_duration=0.1,         # seconds per frame
)
```

That builds a 4-frame looping clip, each frame showing for 0.1 s.

## Building frames by hand

For irregular layouts, add frames individually with explicit source rectangles and
per-frame durations:

```python
attack = loom.Animation("attack", loop=False)
attack.add_frame(loom.Rect(0,   0, 64, 64), duration=0.05)
attack.add_frame(loom.Rect(64,  0, 64, 64), duration=0.05)
attack.add_frame(loom.Rect(128, 0, 64, 64), duration=0.20)  # hold the last frame
```

| `Animation` member | Description |
|--------------------|-------------|
| `Animation(name, loop=True)` | create a clip |
| `add_frame(source, duration=0.1)` | append one frame (`source` is a `Rect`) |
| `add_strip(sheet_w, sheet_h, frame_w, frame_h, cols, rows=1, frame_duration=0.1)` | slice a grid |
| `loop` | whether playback repeats |
| `frame_count()` | number of frames |

## Playing on a sprite

Register clips on a `SpriteNode`, then `play` one by name. The sprite advances the
animation automatically as the scene updates.

```python
hero = loom.SpriteNode(hero_sheet)
hero.add_animation(run)
hero.add_animation(attack)

hero.play("run")     # start looping
# ...
hero.play("attack")  # switch clips
hero.stop()          # freeze on the current frame
```

A typical state-driven update:

```python
def on_update(self, dt):
    moving = loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.Left)
    self.hero.play("run" if moving else "idle")
    self.hero.flip_x = loom.Input.key_down(loom.Key.Left)
```

!!! tip "Animation drives the source rectangle"
    Under the hood, playing a clip sets the sprite's source rect each frame — the same
    mechanism as [`set_source`](sprites.md#sub-rectangles-sprite-sheets). You can mix
    manual `set_source` calls and animations on the same sprite.

## See also

- [Sprites & Textures](sprites.md) — source rectangles and sheets
- [Examples](../examples.md) — `flappy` and the platformer use sprite nodes you can extend with clips
