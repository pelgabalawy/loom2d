# API Summary

A one-page index of the public `loom2d` API. Everything below is importable directly:

```python
import loom2d as loom
```

For full explanations and examples, follow the links to the guides.

## Entry point

| Symbol | Summary |
|--------|---------|
| `Game` | base class for your game; override `on_start`/`on_update`/`on_draw`/`on_stop` — [guide](../getting-started/game-loop.md) |
| `run(game, title="loom2d", width=800, height=600)` | start the game loop; blocks until the window closes |

### `Game` members

`scene`, `physics`, `audio`, `assets`, `clear_color`, `running`, `auto_physics`,
`auto_scene`, `last_draw_calls`. See [The Game Loop](../getting-started/game-loop.md).

For resolution independence: `logical_width`, `logical_height`, `scale_mode`,
`screen_width`, `screen_height`, and the `on_resize(w, h)` hook — see
[Responsive Scaling](../guides/responsive-scaling.md).

## Math — [guide](math.md)

| Type | Summary |
|------|---------|
| `Vec2(x, y)` | 2D vector/point with `length`, `normalized`, `dot`, `lerp`, `rotated`, … |
| `Rect(x, y, w, h)` | rectangle with `contains`, `intersects`, `intersection`, `center`, … |
| `Color(r, g, b, a=1)` | RGBA `[0,1]`; named constants like `Color.cornflower()` |

## Scene — [guide](../guides/scene-graph.md)

| Type | Summary |
|------|---------|
| `Node` | transform + hierarchy: `position`, `x`, `y`, `rotation`, `scale`, `visible`, `add_child`, `remove_from_parent`, `world_position` |
| `SpriteNode(texture)` | draws a texture: `origin`, `tint`, `flip_x/y`, `set_source`, `play` — [guide](../guides/sprites.md) |
| `Scene` | node tree + `camera`: `add`, `remove`, `clear`, `root` |
| `Camera` | `position`, `zoom`, `rotation`, `set_viewport`, `screen_to_world`, `world_to_screen`, `visible_rect` — [guide](../guides/camera.md) |
| `ScaleMode` | `Fit`, `Stretch`, `Expand`, `PixelPerfect` — [responsive scaling](../guides/responsive-scaling.md) |

## Animation — [guide](../guides/animation.md)

| Type | Summary |
|------|---------|
| `Animation(name, loop=True)` | a clip; `add_frame`, `add_strip`, `frame_count` |
| `AnimationFrame` | one frame: `source` (`Rect`), `duration` |

## Text — [guide](../guides/text.md)

| Type | Summary |
|------|---------|
| `Font` | `Font.load(path, pixel_height)`, `measure`, `line_height`, `ascent` |
| `TextNode(font, text="")` | drawable text: `text`, `color`, `align`, `max_width`, `origin`, `size` |
| `TextAlign` | `Left`, `Center`, `Right` |

## Tilemaps — [guide](../guides/tilemaps.md)

| Type | Summary |
|------|---------|
| `Tilemap` | `Tilemap.load(path)` or build in code; layers, tilesets, grid collision, culling |
| `TileLayer` | `name`, `visible`, `opacity`, `at`, `set`, `fill` |
| `Tileset` | gid → texture sub-rect mapping |

## Input — [guide](../guides/input.md)

| Type | Summary |
|------|---------|
| `Input` | static: `key_down`, `key_pressed`, `key_released`, `mouse_position`, `mouse_down`, `mouse_pressed` |
| `Key` | `A`–`Z`, arrows, `Space`, `Enter`, `Escape`, `Tab`, `Shift`, `Ctrl`, `F1`/`F5`/`F12` |
| `MouseButton` | `Left`, `Middle`, `Right` |

## Physics — [guide](../guides/physics.md) · [events](../guides/physics-events.md)

| Type | Summary |
|------|---------|
| `PhysicsWorld(gravity_x=0, gravity_y=980)` | `step`, `create_body`, `destroy_body`, `raycast`, event lists + callbacks |
| `PhysicsBody` | `add_box`, `add_circle`, `tag`, `position`, `linear_velocity`, `set_*`, `apply_impulse`, `apply_force` |
| `BodyType` | `Static`, `Kinematic`, `Dynamic` |
| `ContactPair` | `body_a`, `body_b` |
| `SensorPair` | `sensor`, `visitor` |
| `RaycastHit` | `hit`, `body`, `point`, `normal`, `fraction` |

## Audio — [guide](../guides/audio.md)

| Type | Summary |
|------|---------|
| `AudioEngine` | `play_sound`, `play_music`, `stop_music`, `set_music_volume`, `music_playing`, `set_master_volume`, `initialized` |

## Assets — [guide](../guides/assets.md)

| Type | Summary |
|------|---------|
| `AssetManager` | `texture(path)` (cached), `clear` |
| `Texture` | `width`, `height`, `path` |
