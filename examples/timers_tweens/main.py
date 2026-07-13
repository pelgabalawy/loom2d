"""
loom2d — timers, tweens and saving.

An easing gallery. Eight squares race across the screen, each on a different
easing curve, so you can see what the curves actually feel like: Linear arrives
mechanically, OutBack overshoots and settles, OutBounce lands and bounces.

Everything moving here is one of the three systems this example is about:

  * tweens  — each square is a `game.tweens.to(sprite, "x", ...)`. Nothing in
              this file moves a sprite by hand in on_update(); the tween manager
              owns the motion and drops each tween when it lands.
  * timers  — a repeating timer relaunches the sweep every few seconds, and a
              one-shot staggers each row so they set off in sequence.
  * saving  — the total number of sweeps is written to a JSON save file in the
              directory the OS set aside for this game, so it survives a restart.
              Run it twice and the "all runs" counter keeps climbing.

The pulsing hint text shows two more things: a dotted tween path ("color.a"
reaches into the label's colour and fades its alpha), and chaining — each pulse
starts the next one from its on_complete.

Run:        python examples/timers_tweens/main.py
Smoke test: python examples/timers_tweens/main.py --frames 300
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 640, 360
TRACK_X0, TRACK_X1 = 176, 604       # where the squares run from and to
SWEEP_SECONDS = 1.6                 # how long one crossing takes
RELAUNCH_SECONDS = 2.8              # gap between sweeps
STAGGER_SECONDS = 0.06              # delay between one row setting off and the next

BG = loom.Color(0.09, 0.10, 0.14, 1.0)
INK = loom.Color(1.0, 1.0, 1.0, 0.75)
RAIL = loom.Color(1.0, 1.0, 1.0, 0.07)

# The curves on show, top to bottom, each with the colour of its square.
CURVES = [
    ("Linear",      loom.Ease.Linear,      loom.Color(0.55, 0.58, 0.65, 1)),
    ("InOutSine",   loom.Ease.InOutSine,   loom.Color(0.40, 0.70, 0.95, 1)),
    ("OutCubic",    loom.Ease.OutCubic,    loom.Color(0.35, 0.85, 0.75, 1)),
    ("InExpo",      loom.Ease.InExpo,      loom.Color(0.95, 0.80, 0.35, 1)),
    ("InOutQuart",  loom.Ease.InOutQuart,  loom.Color(0.85, 0.55, 0.95, 1)),
    ("OutBack",     loom.Ease.OutBack,     loom.Color(0.95, 0.55, 0.40, 1)),
    ("OutElastic",  loom.Ease.OutElastic,  loom.Color(0.95, 0.40, 0.55, 1)),
    ("OutBounce",   loom.Ease.OutBounce,   loom.Color(0.55, 0.90, 0.45, 1)),
]


class EaseRow:
    """One curve: a labelled square that runs the track and back again."""

    def __init__(self, game, index, name, easing, color):
        self.easing = easing
        self.index = index
        self.forward = True

        y = 66 + index * 34

        rail = loom.SpriteNode(game.tex)
        rail.tint = RAIL
        rail.origin = loom.Vec2(0.0, 0.5)
        rail.scale = loom.Vec2((TRACK_X1 - TRACK_X0) / 8.0, 0.25)
        rail.position = loom.Vec2(TRACK_X0, y)
        game.scene.add(rail)

        self.sprite = loom.SpriteNode(game.tex)
        self.sprite.tint = color
        self.sprite.origin = loom.Vec2(0.5, 0.5)
        self.sprite.scale = loom.Vec2(2.4, 2.4)
        self.sprite.position = loom.Vec2(TRACK_X0, y)
        game.scene.add(self.sprite)

        if game.font:
            label = loom.Label(game.font, name)
            label.color = INK
            label.offset = loom.Vec2(14, y - 18)
            label.size = loom.Vec2(150, 22)
            label.vcenter = True
            game.ui.add(label)

    def launch(self, game, on_arrival):
        """Send the square across, staggered behind the row above it."""
        target = TRACK_X1 if self.forward else TRACK_X0
        self.forward = not self.forward
        # Every square is animated the same way; only the curve differs. The
        # tween reads the current x as its start, so a relaunch always picks up
        # from wherever the square actually is.
        game.tweens.to(
            self.sprite, "x", target, SWEEP_SECONDS,
            easing=self.easing,
            delay=self.index * STAGGER_SECONDS,
            on_complete=on_arrival,
        )


class Gallery(loom.Game):
    """Owns the assets, the rows, the repeating timer and the save file."""

    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = BG

        png = os.path.join(os.path.dirname(__file__), "_white.png")
        if not os.path.exists(png):
            _write_white_png(png, 8)
        self.tex = self.assets.texture(png)
        self.font = _load_system_font(16)

        # Pick up where the last run left off. A missing or corrupt save reads as
        # the default, so there is no "first run" branch to write.
        self.save = loom.SaveFile("loom2d", "Easing Gallery")
        self.progress = self.save.load({"sweeps": 0})
        self.sweeps_this_run = 0

        self.rows = [EaseRow(self, i, name, easing, color)
                     for i, (name, easing, color) in enumerate(CURVES)]

        self._build_hud()

        # Timers drive the whole demo: one sweep now, another every few seconds.
        self.timers.after(0.4, self.launch_sweep)
        self.timers.every(RELAUNCH_SECONDS, self.launch_sweep)

        self.frames = 0
        self.max_frames = _arg_frames()
        print(f"Easing gallery. Save file: {self.save.path}")

    def launch_sweep(self):
        for row in self.rows:
            # Only the last row to arrive counts the sweep, so the counter tracks
            # crossings rather than squares.
            last = row is self.rows[-1]
            row.launch(self, self._on_arrival if last else None)

    def _on_arrival(self):
        self.sweeps_this_run += 1
        self.progress["sweeps"] += 1

    def _build_hud(self):
        if not self.font:
            self.hud = None
            self.hint = None
            return

        self.hud = loom.Label(self.font, "")
        self.hud.color = INK
        self.hud.offset = loom.Vec2(14, 14)
        self.hud.size = loom.Vec2(500, 22)
        self.ui.add(self.hud)

        self.hint = loom.Label(self.font, "close the window to save")
        self.hint.color = loom.Color(1, 1, 1, 1)
        self.hint.anchor = loom.Vec2(0.5, 1.0)
        self.hint.pivot = loom.Vec2(0.5, 1.0)
        self.hint.offset = loom.Vec2(0, -10)
        self.hint.size = loom.Vec2(300, 22)
        self.hint.align = loom.TextAlign.Center
        self.hint.vcenter = True
        self.ui.add(self.hint)
        self._pulse(0.25)

    def _pulse(self, to_alpha):
        # A dotted path reaches through the label into its colour. on_complete
        # starts the opposite pulse, so the two tweens hand off for ever without
        # a single line in on_update().
        self.tweens.to(
            self.hint, "color.a", to_alpha, 0.9, easing=loom.Ease.InOutSine,
            on_complete=lambda: self._pulse(1.0 if to_alpha < 0.5 else 0.25),
        )

    def on_update(self, dt):
        if self.hud is not None:
            total = self.progress["sweeps"]
            self.hud.text = (f"sweeps  this run {self.sweeps_this_run}  |  all runs {total}"
                             f"   tweens {self.tweens.count}  timers {self.timers.count}")

        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            self.running = False

    def on_stop(self):
        # Persist on the way out. The write is atomic, so a save interrupted by a
        # crash leaves the previous one intact rather than a truncated file.
        self.save.save(self.progress)
        print(f"frames={self.frames} sweeps_this_run={self.sweeps_this_run} "
              f"total_sweeps={self.progress['sweeps']} saved -> {self.save.path}")


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
    import struct
    import zlib

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
    loom.run(Gallery(), title="loom2d — Timers, Tweens & Saving", width=960, height=540)
