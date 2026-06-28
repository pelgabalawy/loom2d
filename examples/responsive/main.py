"""
loom2d — Phase 2.7 responsive scaling demo.

Authors against a fixed *logical* resolution (480x270) and lets the engine map it
onto any window/DPI. Resize the window and the content keeps its aspect ratio with
letterbox/pillarbox bars (the default ``ScaleMode.Fit``). Press Q/W/E/R to switch
scale modes live; the four coloured corner markers + centre dot make the
behaviour of each mode obvious.

  Q = Fit          preserve aspect, letterbox/pillarbox  (default)
  W = Stretch      fill the window, distorting aspect
  E = Expand       preserve aspect, fill by showing more world
  R = PixelPerfect integer-scaled Fit (crisp pixel art)

Run:  python examples/responsive/main.py
Smoke test (no interaction, exits after N frames):
      python examples/responsive/main.py --frames 120
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 480, 270  # 16:9 design resolution


class Responsive(loom.Game):
    def on_start(self):
        # Fixed design resolution — everything below is authored in these units.
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = loom.Color(0.10, 0.11, 0.14, 1.0)

        # Build a white 8x8 texture procedurally so the demo needs zero assets.
        png = os.path.join(os.path.dirname(__file__), "_white.png")
        if not os.path.exists(png):
            _write_white_png(png, 8)
        self.tex = self.assets.texture(png)

        # Four corner markers + a centre marker, placed at the logical bounds so
        # you can see exactly where the design rectangle's edges land.
        m = 12
        corners = [
            (m, m, loom.Color.red()),
            (LOGICAL_W - m, m, loom.Color.green()),
            (m, LOGICAL_H - m, loom.Color.blue()),
            (LOGICAL_W - m, LOGICAL_H - m, loom.Color.yellow()),
            (LOGICAL_W / 2, LOGICAL_H / 2, loom.Color.white()),
        ]
        for x, y, col in corners:
            s = loom.SpriteNode(self.tex)
            s.origin = loom.Vec2(0.5, 0.5)
            s.scale = loom.Vec2(2.0, 2.0)
            s.tint = col
            s.position = loom.Vec2(x, y)
            self.scene.add(s)

        self.frames = 0
        self.max_frames = _arg_frames()
        print(f"Responsive demo: logical {LOGICAL_W}x{LOGICAL_H}, mode=Fit")
        print("Resize the window. Press Q=Fit W=Stretch E=Expand R=PixelPerfect.")

    def on_update(self, dt):
        # Number-row keys aren't in the Key enum yet, so use Q/W/E/R:
        if loom.Input.key_pressed(loom.Key.Q): self._set(loom.ScaleMode.Fit)
        if loom.Input.key_pressed(loom.Key.W): self._set(loom.ScaleMode.Stretch)
        if loom.Input.key_pressed(loom.Key.E): self._set(loom.ScaleMode.Expand)
        if loom.Input.key_pressed(loom.Key.R): self._set(loom.ScaleMode.PixelPerfect)

        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            print(f"screen={self.screen_width}x{self.screen_height} "
                  f"draw_calls={self.last_draw_calls}")
            self.running = False

    def _set(self, mode):
        self.scale_mode = mode
        print(f"scale_mode -> {mode}")

    def on_resize(self, w, h):
        print(f"on_resize: drawable now {w}x{h}")


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
        raw.append(0)  # filter type 0 per scanline
        raw.extend([255, 255, 255, 255] * w)
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)  # 8-bit RGBA
    idat = zlib.compress(bytes(raw))
    with open(path, "wb") as f:
        f.write(sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b""))


if __name__ == "__main__":
    loom.run(Responsive(), title="loom2d — Responsive Scaling",
             width=960, height=540)
