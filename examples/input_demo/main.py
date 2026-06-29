"""
loom2d — Phase 2.8 input breadth demo.

Shows the new input devices working together against a 640x360 logical screen:

  * Gamepad  — left stick moves the player square; the South button (A / Cross)
               changes its colour; right trigger scales it up. Plug a controller
               in at any time (hot-plug). Falls back to WASD/arrows on keyboard.
  * Mouse    — left-drag moves the player to the cursor; the wheel zooms.
  * Touch    — tap to teleport the player; drag to move it (touchscreen / trackpad).
  * Text     — anything you type is appended to a caption (Backspace deletes).
               Text input is enabled on start.

Run:  python examples/input_demo/main.py
Smoke test (no interaction, exits after N frames):
      python examples/input_demo/main.py --frames 120
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 640, 360


class InputDemo(loom.Game):
    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = loom.Color(0.08, 0.09, 0.12, 1.0)

        png = os.path.join(os.path.dirname(__file__), "_white.png")
        if not os.path.exists(png):
            _write_white_png(png, 8)
        self.tex = self.assets.texture(png)

        self.player = loom.SpriteNode(self.tex)
        self.player.origin = loom.Vec2(0.5, 0.5)
        self.player.scale = loom.Vec2(6.0, 6.0)
        self.player.tint = loom.Color.white()
        self.player.position = loom.Vec2(LOGICAL_W / 2, LOGICAL_H / 2)
        self.scene.add(self.player)

        self.size = 6.0
        self.caption = ""
        self._colors = [loom.Color.white(), loom.Color.red(),
                        loom.Color.green(), loom.Color.yellow()]
        self._color_i = 0

        # Try to render an on-screen caption if a system font is available.
        self.font = _load_system_font(18)
        if self.font:
            self.label = loom.TextNode(self.font, "Type something… (gamepad/mouse/touch ready)")
            self.label.color = loom.Color(0.8, 0.85, 0.9, 1.0)
            self.label.position = loom.Vec2(12, 12)
            self.scene.add(self.label)
        else:
            self.label = None

        loom.Input.start_text_input()  # enable typed-character events

        self.frames = 0
        self.max_frames = _arg_frames()
        print(f"Input demo: logical {LOGICAL_W}x{LOGICAL_H}")
        print("Gamepad left-stick / WASD to move, South/A to recolour, "
              "wheel to zoom, type to caption.")

    def on_update(self, dt):
        speed = 220.0 * dt

        # ── Gamepad (pad 0) with keyboard fallback ─────────────────────────────
        if loom.Input.gamepad_connected(0):
            self.player.x += loom.Input.gamepad_axis(loom.GamepadAxis.LeftX, 0) * speed
            self.player.y += loom.Input.gamepad_axis(loom.GamepadAxis.LeftY, 0) * speed
            if loom.Input.gamepad_pressed(loom.GamepadButton.South, 0):
                self._cycle_color()
            trig = loom.Input.gamepad_axis(loom.GamepadAxis.TriggerRight, 0)
            self.size = 6.0 + trig * 8.0
        else:
            if loom.Input.key_down(loom.Key.A) or loom.Input.key_down(loom.Key.Left):
                self.player.x -= speed
            if loom.Input.key_down(loom.Key.D) or loom.Input.key_down(loom.Key.Right):
                self.player.x += speed
            if loom.Input.key_down(loom.Key.W) or loom.Input.key_down(loom.Key.Up):
                self.player.y -= speed
            if loom.Input.key_down(loom.Key.S) or loom.Input.key_down(loom.Key.Down):
                self.player.y += speed
            if loom.Input.key_pressed(loom.Key.Space):
                self._cycle_color()

        # ── Mouse: wheel zooms the sprite, left-drag pulls it to the cursor ────
        self.size = max(2.0, self.size + loom.Input.mouse_wheel().y * 0.5)
        self.player.scale = loom.Vec2(self.size, self.size)
        if loom.Input.mouse_down(loom.MouseButton.Left):
            self.player.position = loom.Input.mouse_position()

        # ── Touch: tap to teleport, drag to follow the first finger ────────────
        for t in loom.Input.touches_began():
            self.player.position = t.position
        active = loom.Input.touches()
        if active:
            self.player.position = active[0].position

        # ── Text input → caption ───────────────────────────────────────────────
        typed = loom.Input.text_input()
        if typed:
            self.caption += typed
        if loom.Input.key_pressed(loom.Key.Backspace) and self.caption:
            self.caption = self.caption[:-1]
        if self.label is not None:
            self.label.text = self.caption or "Type something…"

        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            print(f"pads={loom.Input.gamepad_count()} "
                  f"touches={loom.Input.touch_count()} "
                  f"draw_calls={self.last_draw_calls}")
            self.running = False

    def _cycle_color(self):
        self._color_i = (self._color_i + 1) % len(self._colors)
        self.player.tint = self._colors[self._color_i]


def _load_system_font(px):
    candidates = [
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    ]
    for path in candidates:
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
    loom.run(InputDemo(), title="loom2d — Input Breadth",
             width=960, height=540)
