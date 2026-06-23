"""
text_demo — Phase 2.6 text rendering.

Shows the Font / TextNode API:
  * a baked TTF atlas (one draw call per font, whatever the string length)
  * left / center / right alignment
  * word-wrap at a pixel width
  * tinting, scaling and rotation (TextNode is a Node like any other)

Run interactively:   python main.py
Render smoke test (auto-terminates, prints diagnostics — needs a display):
    python main.py --frames 300

Uses a system font; pass an explicit path with:  python main.py --font C:/path/to.ttf
"""
import os, sys, math

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

WIN_W, WIN_H = 900, 600

FRAMES = None
if '--frames' in sys.argv:
    FRAMES = int(sys.argv[sys.argv.index('--frames') + 1])

FONT_OVERRIDE = None
if '--font' in sys.argv:
    FONT_OVERRIDE = sys.argv[sys.argv.index('--font') + 1]


def find_system_font():
    """Best-effort path to a TTF that exists on this OS (None if none found)."""
    if FONT_OVERRIDE:
        return FONT_OVERRIDE
    candidates = [
        # Windows
        r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\segoeui.ttf",
        r"C:\Windows\Fonts\calibri.ttf",
        # macOS
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        # Linux
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    return None


class TextDemo(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color(0.10, 0.11, 0.15, 1.0)

        font_path = find_system_font()
        if not font_path:
            print("text_demo: no system font found. Pass one with --font <path/to.ttf>.")
            self.running = False
            return
        print(f"text_demo: using font {font_path}")

        self.font_big = loom.Font.load(font_path, 48)
        self.font     = loom.Font.load(font_path, 24)
        self.frame = 0

        # Title, centered horizontally on the window.
        title = loom.TextNode(self.font_big, "loom2d text rendering")
        title.color = loom.Color(0.95, 0.85, 0.45, 1.0)
        title.origin = loom.Vec2(0.5, 0.0)          # anchor at top-center
        title.position = loom.Vec2(WIN_W / 2, 30)
        self.scene.add(title)

        # Three alignment samples sharing one wrap width.
        sample = "The quick brown fox jumps over the lazy dog."
        for i, (al, label, col) in enumerate([
            (loom.TextAlign.Left,   "left",   loom.Color(0.6, 0.85, 1.0, 1.0)),
            (loom.TextAlign.Center, "center", loom.Color(0.7, 1.0, 0.7, 1.0)),
            (loom.TextAlign.Right,  "right",  loom.Color(1.0, 0.7, 0.7, 1.0)),
        ]):
            n = loom.TextNode(self.font, f"[{label}] {sample}")
            n.align = al
            n.max_width = 360
            n.color = col
            n.position = loom.Vec2(60, 130 + i * 110)
            self.scene.add(n)

        # A spinning, scaled label anchored at its center.
        self.spin = loom.TextNode(self.font_big, "spin!")
        self.spin.color = loom.Color(1.0, 0.6, 0.9, 1.0)
        self.spin.origin = loom.Vec2(0.5, 0.5)
        self.spin.position = loom.Vec2(WIN_W - 160, WIN_H - 140)
        self.scene.add(self.spin)

        # A live readout, updated every frame.
        self.clock = loom.TextNode(self.font, "")
        self.clock.color = loom.Color.white()
        self.clock.position = loom.Vec2(60, WIN_H - 50)
        self.scene.add(self.clock)

        self.time = 0.0

    def on_update(self, dt):
        if not self.running:
            return
        self.frame += 1
        self.time += dt
        self.spin.rotation = self.time
        self.spin.scale = loom.Vec2(1.0 + 0.3 * math.sin(self.time * 2),
                                    1.0 + 0.3 * math.sin(self.time * 2))
        self.clock.text = f"elapsed {self.time:5.1f}s   draw_calls {self.last_draw_calls}"

        if loom.Input.key_pressed(loom.Key.Escape):
            self.running = False

        if FRAMES is not None:
            if self.frame % 60 == 0 or self.frame == FRAMES:
                print(f"frame {self.frame:4d}  draw_calls {self.last_draw_calls}")
            if self.frame >= FRAMES:
                self.running = False


if __name__ == '__main__':
    loom.run(TextDemo(), title="loom2d — text_demo", width=WIN_W, height=WIN_H)
