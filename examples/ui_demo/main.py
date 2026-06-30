"""
loom2d — Phase 2.9 UI toolkit demo.

A screen-space UI laid out over a scrolling world, showing every widget:

  * Panel  — a translucent backdrop with a border.
  * Label  — text that stays pinned regardless of camera movement.
  * Button — hover/press states + an on_clicked callback (changes the world tint
             and bumps a counter label). Buttons are focusable (click to focus).
  * Image  — a textured widget anchored to a corner.
  * Grid   — a row of colour-swatch buttons arranged automatically.

The world (a grid of sprites) scrolls left-right under the UI to prove the UI
layer is unaffected by the world camera. Move the mouse over the buttons and
click them.

Run:        python examples/ui_demo/main.py
Smoke test: python examples/ui_demo/main.py --frames 120
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 640, 360


class UIDemo(loom.Game):
    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = loom.Color(0.07, 0.08, 0.11, 1.0)

        png = os.path.join(os.path.dirname(__file__), "_white.png")
        if not os.path.exists(png):
            _write_white_png(png, 8)
        self.tex = self.assets.texture(png)

        # ── A scrolling world behind the UI (proves UI ignores the camera) ─────
        self.world_tint = loom.Color(0.30, 0.45, 0.70, 1.0)
        self.sprites = []
        for gy in range(6):
            for gx in range(10):
                s = loom.SpriteNode(self.tex)
                s.scale = loom.Vec2(5, 5)
                s.position = loom.Vec2(40 + gx * 64, 40 + gy * 56)
                s.tint = self.world_tint
                self.scene.add(s)
                self.sprites.append(s)
        self.t = 0.0

        self.font = _load_system_font(18)
        self.clicks = 0
        self._build_ui()

        self.frames = 0
        self.max_frames = _arg_frames()
        print(f"UI demo: logical {LOGICAL_W}x{LOGICAL_H}. Click the buttons.")

    def _build_ui(self):
        ui = self.ui

        # Top bar panel pinned across the top (fill width, fixed height).
        bar = loom.Panel(loom.Color(0.0, 0.0, 0.0, 0.55))
        bar.size = loom.Vec2(0, 44)          # width 0 -> fill parent width
        bar.border_color = loom.Color(1, 1, 1, 0.15)
        bar.border_width = 1
        ui.add(bar)

        if self.font:
            title = loom.Label(self.font, "loom2d UI")
            title.color = loom.Color.white()
            title.vcenter = True
            title.anchor = loom.Vec2(0, 0)
            title.offset = loom.Vec2(12, 0)
            title.size = loom.Vec2(200, 44)
            bar.add_child(title)

            self.counter = loom.Label(self.font, "clicks: 0")
            self.counter.color = loom.Color(0.7, 0.85, 1.0, 1.0)
            self.counter.align = loom.TextAlign.Right
            self.counter.vcenter = True
            self.counter.anchor = loom.Vec2(1, 0)
            self.counter.pivot = loom.Vec2(1, 0)
            self.counter.offset = loom.Vec2(-12, 0)
            self.counter.size = loom.Vec2(160, 44)
            bar.add_child(self.counter)
        else:
            self.counter = None

        # A centred action button.
        btn = loom.Button(self.font, "Click me")
        btn.anchor = loom.Vec2(0.5, 0.5)
        btn.pivot = loom.Vec2(0.5, 0.5)
        btn.size = loom.Vec2(160, 48)
        btn.on_clicked = self._on_click
        ui.add(btn)

        # A grid of colour swatches pinned to the bottom-left.
        grid = loom.Grid(4, loom.Vec2(8, 8))
        grid.anchor = loom.Vec2(0, 1)
        grid.pivot = loom.Vec2(0, 1)
        grid.offset = loom.Vec2(12, -12)
        grid.size = loom.Vec2(260, 60)
        swatches = [
            ("Blue",  loom.Color(0.30, 0.45, 0.70, 1.0)),
            ("Green", loom.Color(0.30, 0.65, 0.40, 1.0)),
            ("Rust",  loom.Color(0.75, 0.40, 0.25, 1.0)),
            ("Plum",  loom.Color(0.55, 0.35, 0.65, 1.0)),
        ]
        for label, color in swatches:
            sw = loom.Button(self.font, label)
            sw.bg = color
            sw.bg_hover = loom.Color(min(color.r + 0.12, 1), min(color.g + 0.12, 1),
                                     min(color.b + 0.12, 1), 1.0)
            sw.on_clicked = (lambda c=color: self._set_world_tint(c))
            grid.add_child(sw)
        ui.add(grid)

        # An image widget pinned to the bottom-right corner.
        img = loom.Image(self.tex)
        img.tint = loom.Color(1.0, 0.85, 0.2, 1.0)
        img.anchor = loom.Vec2(1, 1)
        img.pivot = loom.Vec2(1, 1)
        img.offset = loom.Vec2(-12, -12)
        img.size = loom.Vec2(40, 40)
        ui.add(img)

    def _on_click(self):
        self.clicks += 1
        if self.counter is not None:
            self.counter.text = f"clicks: {self.clicks}"

    def _set_world_tint(self, color):
        self.world_tint = color
        for s in self.sprites:
            s.tint = color

    def on_update(self, dt):
        # Scroll the world camera; the UI must not move with it.
        self.t += dt
        import math
        self.scene.camera.position = loom.Vec2(
            LOGICAL_W / 2 + math.sin(self.t * 0.6) * 90, LOGICAL_H / 2)

        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            print(f"clicks={self.clicks} draw_calls={self.last_draw_calls}")
            self.running = False


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


def _write_white_png(path, size):
    """Write a minimal opaque-white RGBA PNG with no external deps."""
    import struct, zlib
    w = h = size
    raw = bytearray()
    for _ in range(h):
        raw.append(0)
        raw.extend([255, 255, 255, 255] * w)

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    idat = zlib.compress(bytes(raw))
    with open(path, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b""))


if __name__ == "__main__":
    loom.run(UIDemo(), title="loom2d — UI Toolkit", width=960, height=540)
