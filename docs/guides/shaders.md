# Shaders, Blend Modes & Canvases

Three ways to change how something is drawn, rather than what is drawn.

* **`blend`** decides how a drawable's pixels combine with what is already there
  — additive glows, multiply shadows.
* **`shader`** replaces the fragment program used to draw it, so you can write
  the pixel maths yourself in GLSL.
* **`Canvas`** is an off-screen render target: draw a node tree into a texture,
  then use that texture like any other.

All three work on the same drawables you already have — sprites, text, tilemaps
and UI widgets.

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        fire = loom.SpriteNode(self.assets.texture("flame.png"))
        fire.blend = loom.BlendMode.Add        # a glow that adds light
        self.scene.add(fire)
```

## Blend modes

Set `blend` on any drawable. It applies to that drawable's own pixels — not to
its children, which carry their own.

| Mode | What it does | Reach for it when |
|------|--------------|-------------------|
| `BlendMode.Alpha` | normal transparency (the default) | almost always |
| `BlendMode.Add` | adds light to what's underneath | fire, lasers, glows, magic |
| `BlendMode.Multiply` | darkens what's underneath | shadows, colour washes, dirt |
| `BlendMode.Screen` | lightens, but never blows out to white | soft glows, fog, light shafts |
| `BlendMode.Replace` | no blending at all | overwriting a target wholesale |

`Add` and `Multiply` still respect the drawable's alpha, so fading a sprite's
`tint.a` down to zero fades the effect out rather than leaving it at full
strength — which means you can tween a glow in and out like anything else.

`Replace` is the exception: with blending switched off, **the alpha channel is
ignored**, so a soft-edged texture draws as a hard rectangle. That is what "no
blending" means, and it is rarely what you want on a sprite.

```python
# A glow that pulses, by tweening the alpha of an additive sprite.
glow.blend = loom.BlendMode.Add
self.tweens.to(glow, "tint.a", 0.2, 0.8, loom.Ease.InOutSine)
```

## Shaders

A loom2d shader is **one GLSL function**, `effect()`. You do not write a whole
program: loom2d wraps yours in the right header for whichever GLSL dialect the
platform wants (desktop OpenGL, or GLES3 on mobile), so a single shader source
runs everywhere.

```python
shader = loom.Shader("""
    vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
        vec4 c = texture(tex, uv);
        float grey = dot(c.rgb, vec3(0.299, 0.587, 0.114));
        return vec4(vec3(grey), c.a) * color;
    }
""")

sprite.shader = shader     # any sprite, text node, tilemap or widget
```

Inside `effect()`:

| Name | Is |
|------|-----|
| `color` | the drawable's tint (`sprite.tint`, `label.color`, …) |
| `tex` | the drawable's texture |
| `uv` | the texture coordinate of this pixel |

Return the colour you want. Multiplying by `color` at the end keeps tint and
fade working normally — if you drop it, the drawable stops responding to its own
`tint`.

!!! warning "Build shaders in `on_start()`, not at import time"
    A shader is compiled on the GPU, so it needs a window to exist. Creating one
    at module scope raises `RuntimeError: no renderer yet`. The same goes for
    `Canvas`.

If the GLSL fails to compile you get a `RuntimeError` carrying the driver's own
error message, with line numbers pointing at **your** source rather than at the
wrapper.

### Uniforms

Declare a `uniform` in the source and set it from Python. `float`, `vec2`,
`vec3`, `vec4` and `mat4` are supported.

```python
shader = loom.Shader("""
    uniform float u_time;
    uniform vec4  u_flash;

    vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
        vec4 c = texture(tex, uv);
        float pulse = 0.5 + 0.5 * sin(u_time * 6.0);
        return mix(c, u_flash, pulse * u_flash.a) * color;
    }
""")

class MyGame(loom.Game):
    def on_update(self, dt):
        self.elapsed += dt
        shader.set("u_time", self.elapsed)                  # a float
        shader.set("u_flash", loom.Color(1, 0, 0, 0.5))     # a Color -> vec4
        shader.set("u_centre", loom.Vec2(320, 180))         # a Vec2   -> vec2
```

`set()` takes a number, a `Vec2`, a `Color`, or any sequence of floats (16 for a
`mat4`). Setting a uniform the source doesn't declare raises, and tells you which
names it does declare — `shader.uniforms` lists them.

Uniforms belong to the **shader**, not the drawable. Two sprites sharing a shader
share its uniforms; the batcher snapshots the values at the moment each drawable
is submitted, so you can set a uniform, draw, change it, and draw again in the
same frame and both come out right. To give each sprite its own value, give each
its own `Shader`.

## Canvases

A `Canvas` is a texture you can render into.

```python
class MyGame(loom.Game):
    def on_start(self):
        self.minimap = loom.Canvas(128, 128)
        self.minimap.clear_color = loom.Color(0, 0, 0, 0.5)

        # The tree to render. It is NOT added to the scene — the only way it
        # reaches the screen is through the canvas.
        self.blips = loom.Node()
        ...

        # Show the canvas like any other texture.
        view = loom.SpriteNode(self.minimap.texture)
        view.position = loom.Vec2(560, 60)
        self.ui_layer.add(view)

    def on_update(self, dt):
        self.render_to_canvas(self.minimap, self.blips)
```

`game.render_to_canvas(canvas, root)` clears the canvas to its `clear_color` and
draws `root` and its children into it. With no camera the canvas is drawn in its
own pixel space, `(0, 0)` at the top-left corner — the same convention as the UI
layer. Pass one to see the tree through a camera instead:

```python
self.render_to_canvas(self.minimap, self.world_root, self.minimap_camera)
```

Good for minimaps, portals, security cameras, reflections, and baking an
expensive backdrop once instead of redrawing it every frame.

!!! warning "Render canvases from `on_update()`, not `on_draw()`"
    A canvas is its own render pass, and render passes cannot nest. By the time
    `on_draw()` runs, the frame's pass is already open, so rendering a canvas
    from there raises. `on_update()` is the right place — the canvas is ready by
    the time the frame is drawn.

## Screen shaders

Set `game.post_process` and the whole finished frame — scene, UI, transitions —
is rendered into a canvas and then drawn to the window through your shader.

```python
class MyGame(loom.Game):
    def on_start(self):
        self.crt = loom.Shader("""
            uniform vec2 u_resolution;

            vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
                vec4 c = texture(tex, uv);
                float scan = 0.88 + 0.12 * sin(uv.y * u_resolution.y * 3.1416);
                return vec4(c.rgb * scan, 1.0) * color;
            }
        """)
        self.post_process = self.crt

    def on_update(self, dt):
        self.crt.set("u_resolution", loom.Vec2(self.screen_width, self.screen_height))
```

Here `tex` is the frame itself and `uv` runs `(0,0)`–`(1,1)` across the window,
so this is where CRT filters, colour grading, vignettes, bloom and screen-shake
live. Setting `post_process` back to `None` removes the extra pass entirely.

## Cost

Drawables are batched by draw state, so sprites sharing a texture, a shader, a
blend mode **and** a set of uniform values collapse into one draw call. Changing
any of them starts a new batch — so a scene where every sprite has a different
blend mode costs a draw call per sprite. Group them and the cost disappears.

`game.last_draw_calls` reports the count for the last frame.

## See it running

```bash
python examples/shaders/main.py
```

The five blend modes side by side, a sprite running a custom shader, a canvas
rendered off-screen and drawn back, and a CRT screen shader on the space bar.
