"""
loom2d — shaders, blend modes and canvases.

Four things, all on screen at once:

  * blend modes — the same pair of overlapping glows drawn five ways. Add builds
                  light where they cross, Multiply digs a hole in the panel,
                  Screen lightens without blowing out, Replace ignores what is
                  under it entirely.
  * shaders     — the crest sprite runs a GLSL effect() with a `u_time` uniform,
                  set fresh every frame from on_update().
  * canvases    — the little framed panel on the right is a Canvas: a node tree
                  (which is NOT in the scene) rendered off-screen each frame and
                  then drawn back as an ordinary sprite. Its red corner marker
                  sits top-left, which is how you can see a render target comes
                  out the right way up.
  * post_process— SPACE toggles a CRT shader over the whole finished frame:
                  barrel warp, scanlines, chromatic split, vignette.

Run:        python examples/shaders/main.py
Smoke test: python examples/shaders/main.py --frames 240
Keys:       SPACE toggle CRT   ESC quit
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 640, 360

BG = loom.Color(0.07, 0.08, 0.12, 1.0)
INK = loom.Color(1.0, 1.0, 1.0, 0.80)
DIM = loom.Color(1.0, 1.0, 1.0, 0.45)
PANEL = loom.Color(0.42, 0.44, 0.52, 1.0)   # mid grey, so Multiply has room to darken

# The five modes, left to right, with the two glow colours that cross in each.
BLEND_MODES = [
    ("Alpha",    loom.BlendMode.Alpha),
    ("Add",      loom.BlendMode.Add),
    ("Multiply", loom.BlendMode.Multiply),
    ("Screen",   loom.BlendMode.Screen),
    ("Replace",  loom.BlendMode.Replace),
]
GLOW_A = loom.Color(1.00, 0.30, 0.35, 1.0)
GLOW_B = loom.Color(0.30, 0.65, 1.00, 1.0)

# ── The shaders ─────────────────────────────────────────────────────────────
# A loom2d shader is one effect() function. loom2d wraps it in the right GLSL
# header for the backend, so the same source runs on desktop GL and mobile GLES3.

SPRITE_EFFECT = """
uniform float u_time;

vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
    vec4 c = texture(tex, uv);
    // Sweep a band of colour across the sprite, keeping its own alpha shape.
    float wave = 0.5 + 0.5 * sin(u_time * 2.5 - uv.x * 6.283 + uv.y * 2.0);
    vec3 hot = vec3(1.0, 0.55 + 0.45 * wave, 0.15 + 0.5 * wave);
    c.rgb = mix(c.rgb, hot, 0.85 * wave);
    return c * color;
}
"""

CRT_EFFECT = """
uniform float u_time;
uniform vec2  u_resolution;

vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
    // Bulge the picture the way a curved tube does.
    vec2 centred = uv * 2.0 - 1.0;
    centred *= 1.0 + 0.045 * dot(centred, centred);
    vec2 warped = centred * 0.5 + 0.5;

    // Past the edge of the glass there is no picture, only the bezel.
    if (warped.x < 0.0 || warped.x > 1.0 || warped.y < 0.0 || warped.y > 1.0) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    // Split the channels a hair apart, the way a misaligned tube does.
    float split = 0.0018;
    vec3 rgb = vec3(texture(tex, warped + vec2(split, 0.0)).r,
                    texture(tex, warped).g,
                    texture(tex, warped - vec2(split, 0.0)).b);

    // Scanlines, plus a bright band rolling slowly down the screen.
    float scan = 0.88 + 0.12 * sin(warped.y * u_resolution.y * 3.1416);
    float roll = 0.97 + 0.03 * sin(warped.y * 8.0 - u_time * 2.0);
    rgb *= scan * roll;

    // Vignette.
    rgb *= 1.0 - 0.45 * dot(centred, centred) * 0.5;

    return vec4(rgb, 1.0) * color;
}
"""


class BlendGroup:
    """One blend mode: two glows crossing over a grey panel, with a caption."""

    CELL_W = 118
    PANEL_H = 96

    def __init__(self, game, index, name, mode):
        x = 22 + index * self.CELL_W
        y = 96

        panel = loom.SpriteNode(game.white)
        panel.origin = loom.Vec2(0.0, 0.0)
        panel.tint = PANEL
        panel.position = loom.Vec2(x, y)
        panel.scale = loom.Vec2((self.CELL_W - 12) / 8.0, self.PANEL_H / 8.0)
        game.scene.add(panel)

        # Both glows carry the blend mode — it is a property of the drawable, so
        # nothing here has to touch the renderer or order the draws by hand.
        cx = x + (self.CELL_W - 12) * 0.5
        cy = y + self.PANEL_H * 0.5
        for tint, dx in ((GLOW_A, -16), (GLOW_B, 16)):
            glow = loom.SpriteNode(game.glow)
            glow.tint = tint
            glow.blend = mode
            glow.position = loom.Vec2(cx + dx, cy)
            glow.scale = loom.Vec2(0.85, 0.85)
            game.scene.add(glow)

        if game.font:
            caption = loom.Label(game.font, name)
            caption.color = INK
            caption.offset = loom.Vec2(x, y + self.PANEL_H + 6)
            caption.size = loom.Vec2(self.CELL_W - 12, 20)
            caption.align = loom.TextAlign.Center
            caption.vcenter = True
            game.ui.add(caption)


class ShaderCrest:
    """A sprite drawn through a custom GLSL effect, fed a time uniform."""

    def __init__(self, game):
        self.shader = loom.Shader(SPRITE_EFFECT)

        self.sprite = loom.SpriteNode(game.glow)
        self.sprite.shader = self.shader
        self.sprite.position = loom.Vec2(112, 268)
        self.sprite.scale = loom.Vec2(1.5, 1.5)
        game.scene.add(self.sprite)

        if game.font:
            caption = loom.Label(game.font, "sprite.shader")
            caption.color = DIM
            caption.offset = loom.Vec2(52, 318)
            caption.size = loom.Vec2(120, 20)
            caption.align = loom.TextAlign.Center
            game.ui.add(caption)

    def update(self, elapsed):
        # Uniforms are set on the shader, not the sprite: two sprites can share
        # one shader, and the batcher snapshots the values per draw.
        self.shader.set("u_time", elapsed)
        self.sprite.rotation = math.sin(elapsed) * 0.25


class MiniCanvas:
    """A node tree rendered off-screen, then drawn back as a sprite.

    The tree deliberately lives outside the scene: the only way it reaches the
    screen is through the canvas. The red marker sits at the canvas's top-left
    corner, so if a render target ever came out upside-down you would see it
    immediately.
    """

    SIZE = 96

    def __init__(self, game):
        self.canvas = loom.Canvas(self.SIZE, self.SIZE)
        self.canvas.clear_color = loom.Color(0.16, 0.17, 0.24, 1.0)

        self.root = loom.Node("canvas-root")

        marker = loom.SpriteNode(game.white)      # top-left corner marker
        marker.origin = loom.Vec2(0.0, 0.0)
        marker.tint = loom.Color(1.0, 0.35, 0.35, 1.0)
        marker.position = loom.Vec2(4, 4)
        marker.scale = loom.Vec2(1.6, 1.6)
        self.root.add_child(marker)

        self.spinner = loom.SpriteNode(game.glow) # something obviously animated
        self.spinner.tint = loom.Color(0.55, 0.95, 0.70, 1.0)
        self.spinner.position = loom.Vec2(self.SIZE * 0.5, self.SIZE * 0.5)
        self.spinner.scale = loom.Vec2(0.8, 0.8)
        self.root.add_child(self.spinner)

        self.bar = loom.SpriteNode(game.white)    # a bottom edge, for orientation
        self.bar.origin = loom.Vec2(0.0, 1.0)
        self.bar.tint = loom.Color(0.40, 0.55, 1.00, 1.0)
        self.bar.position = loom.Vec2(0, self.SIZE)
        self.bar.scale = loom.Vec2(self.SIZE / 8.0, 0.75)
        self.root.add_child(self.bar)

        # The canvas is shown like any other texture.
        frame = loom.SpriteNode(game.white)
        frame.origin = loom.Vec2(0.5, 0.5)
        frame.tint = loom.Color(1, 1, 1, 0.20)
        frame.position = loom.Vec2(528, 268)
        frame.scale = loom.Vec2((self.SIZE + 6) / 8.0, (self.SIZE + 6) / 8.0)
        game.scene.add(frame)

        self.view = loom.SpriteNode(self.canvas.texture)
        self.view.position = loom.Vec2(528, 268)
        game.scene.add(self.view)

        if game.font:
            caption = loom.Label(game.font, "Canvas (render-to-texture)")
            caption.color = DIM
            caption.offset = loom.Vec2(438, 322)
            caption.size = loom.Vec2(180, 20)
            caption.align = loom.TextAlign.Center
            game.ui.add(caption)

    def update(self, game, elapsed):
        self.spinner.rotation = elapsed * 1.5
        self.spinner.scale = loom.Vec2(0.6 + 0.2 * math.sin(elapsed * 3),
                                       0.6 + 0.2 * math.sin(elapsed * 3))
        # Rendering into a canvas is its own pass, so it happens here in
        # on_update — not in on_draw, where the frame's pass is already open.
        game.render_to_canvas(self.canvas, self.root)


class ScreenShader:
    """The whole frame, through one shader. Toggled with SPACE."""

    def __init__(self, game):
        self.game = game
        self.shader = loom.Shader(CRT_EFFECT)
        self.enabled = False

    def toggle(self):
        self.enabled = not self.enabled
        # Assigning the shader is the whole switch: with it set, the run loop
        # renders the frame into a canvas and draws that through the shader.
        self.game.post_process = self.shader if self.enabled else None

    def update(self, elapsed):
        if not self.enabled:
            return
        self.shader.set("u_time", elapsed)
        self.shader.set("u_resolution",
                        loom.Vec2(self.game.screen_width, self.game.screen_height))


class Showcase(loom.Game):
    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = BG

        here = os.path.dirname(__file__)
        white_png = os.path.join(here, "_white.png")
        glow_png = os.path.join(here, "_glow.png")
        if not os.path.exists(white_png):
            _write_png(white_png, 8, lambda x, y, s: (255, 255, 255, 255))
        if not os.path.exists(glow_png):
            _write_png(glow_png, 64, _glow_pixel)

        self.white = self.assets.texture(white_png)
        self.glow = self.assets.texture(glow_png)
        self.font = _load_system_font(15)

        self.elapsed = 0.0

        for i, (name, mode) in enumerate(BLEND_MODES):
            BlendGroup(self, i, name, mode)

        self.crest = ShaderCrest(self)
        self.mini = MiniCanvas(self)
        self.crt = ScreenShader(self)

        self._build_hud()

        self.frames = 0
        self.max_frames = _arg_frames()

    def _build_hud(self):
        if not self.font:
            self.hud = None
            return

        title = loom.Label(self.font, "shaders  ·  blend modes  ·  canvases")
        title.color = INK
        title.offset = loom.Vec2(22, 22)
        title.size = loom.Vec2(400, 22)
        self.ui.add(title)

        self.hud = loom.Label(self.font, "")
        self.hud.color = DIM
        self.hud.anchor = loom.Vec2(0.0, 1.0)
        self.hud.pivot = loom.Vec2(0.0, 1.0)
        self.hud.offset = loom.Vec2(22, -12)
        self.hud.size = loom.Vec2(600, 22)
        self.ui.add(self.hud)

    def on_update(self, dt):
        self.elapsed += dt

        if loom.Input.key_pressed(loom.Key.Space):
            self.crt.toggle()

        self.crest.update(self.elapsed)
        self.mini.update(self, self.elapsed)
        self.crt.update(self.elapsed)

        if self.hud is not None:
            state = "on" if self.crt.enabled else "off"
            self.hud.text = (f"SPACE: CRT screen shader [{state}]"
                             f"    draw calls {self.last_draw_calls}")

        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            self.running = False

    def on_stop(self):
        print(f"frames={self.frames} draw_calls={self.last_draw_calls} "
              f"crt={'on' if self.crt.enabled else 'off'}")


def _glow_pixel(x, y, size):
    """A soft radial glow: white in the middle, fading to transparent."""
    half = size / 2.0
    dx = (x - half + 0.5) / half
    dy = (y - half + 0.5) / half
    d = math.sqrt(dx * dx + dy * dy)
    a = max(0.0, 1.0 - d)
    a = a * a  # tighter falloff than linear
    return (255, 255, 255, int(255 * a))


def _load_system_font(px):
    for path in (
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ):
        if os.path.exists(path):
            try:
                return loom.Font.load(path, px)
            except Exception:
                pass
    return None


def _arg_frames():
    if "--frames" in sys.argv:
        i = sys.argv.index("--frames")
        if i + 1 < len(sys.argv):
            return int(sys.argv[i + 1])
    return 0


def _write_png(path, size, pixel):
    """Write an RGBA PNG, pixel(x, y, size) -> (r, g, b, a). No external deps."""
    import struct
    import zlib

    raw = bytearray()
    for y in range(size):
        raw.append(0)  # filter type: none
        for x in range(size):
            raw.extend(pixel(x, y, size))

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    idat = zlib.compress(bytes(raw))
    with open(path, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b""))


if __name__ == "__main__":
    loom.run(Showcase(), title="loom2d — Shaders, Blend Modes & Canvases",
             width=960, height=540)
