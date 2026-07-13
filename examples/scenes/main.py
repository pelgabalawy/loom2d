"""
loom2d — Phase 2.10 scene management demo.

Three scenes, and every way of moving between them:

  * MenuScene   — a title and a Play button. Play *switches* to the level, so the
                  menu is gone: switch_to() replaces the active scene.
  * LevelScene  — a bouncing sprite and a HUD. Press P to *push* the pause menu,
                  which lays it on top without disturbing the level underneath.
  * PauseScene  — Resume pops back to the level, exactly as it was left (the
                  sprite has not moved). Quit switches back to the menu.

Every move is wrapped in a Fade, so the swap happens behind a black screen.

Note how each scene owns its own `ui`: the menu's buttons and the level's HUD
never have to share one canvas, and the pause menu's widgets vanish with it when
it is popped.

Run:        python examples/scenes/main.py
Smoke test: python examples/scenes/main.py --frames 300
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "python"))

import loom2d as loom

LOGICAL_W, LOGICAL_H = 640, 360

BG_MENU  = loom.Color(0.10, 0.11, 0.16, 1.0)
BG_LEVEL = loom.Color(0.07, 0.13, 0.11, 1.0)
ACCENT   = loom.Color(0.35, 0.75, 0.95, 1.0)


class MenuScene(loom.Scene):
    """Title screen. Play replaces this scene with the level."""

    def on_enter(self):
        self.game.clear_color = BG_MENU

        if self.game.font:
            title = loom.Label(self.game.font, "loom2d")
            title.color = ACCENT
            title.anchor = loom.Vec2(0.5, 0.35)
            title.pivot = loom.Vec2(0.5, 0.5)
            title.size = loom.Vec2(220, 40)
            title.align = loom.TextAlign.Center
            title.vcenter = True
            self.ui.add(title)

            hint = loom.Label(self.game.font, "scene management demo")
            hint.color = loom.Color(1, 1, 1, 0.45)
            hint.anchor = loom.Vec2(0.5, 0.35)
            hint.pivot = loom.Vec2(0.5, 0.5)
            hint.offset = loom.Vec2(0, 34)
            hint.size = loom.Vec2(300, 24)
            hint.align = loom.TextAlign.Center
            hint.vcenter = True
            self.ui.add(hint)

        play = _button("Play", self.game.font, self._play)
        play.anchor = loom.Vec2(0.5, 0.62)
        play.pivot = loom.Vec2(0.5, 0.5)
        self.ui.add(play)

    def _play(self):
        # Replace the menu with the level, behind a fade.
        self.game.scenes.switch_to(LevelScene(), loom.Fade(0.35))


class LevelScene(loom.Scene):
    """The 'game'. Push the pause menu over it with P."""

    def on_enter(self):
        self.game.clear_color = BG_LEVEL

        self.hero = loom.SpriteNode(self.game.tex)
        self.hero.scale = loom.Vec2(6, 6)
        self.hero.position = loom.Vec2(80, 180)
        self.hero.tint = ACCENT
        self.add(self.hero)

        self.vel = loom.Vec2(150, 95)

        if self.game.font:
            self.hud = loom.Label(self.game.font, "")
            self.hud.color = loom.Color(1, 1, 1, 0.8)
            self.hud.offset = loom.Vec2(12, 10)
            self.hud.size = loom.Vec2(400, 24)
            self.ui.add(self.hud)
        else:
            self.hud = None

        self.elapsed = 0.0

    def on_update(self, dt):
        # This stops being called the moment the pause menu is pushed on top —
        # that is the whole point of push(): the level freezes where it is.
        self.elapsed += dt

        x = self.hero.x + self.vel.x * dt
        y = self.hero.y + self.vel.y * dt
        if x < 20 or x > LOGICAL_W - 20:
            self.vel.x = -self.vel.x
            x = self.hero.x
        if y < 20 or y > LOGICAL_H - 20:
            self.vel.y = -self.vel.y
            y = self.hero.y
        self.hero.x, self.hero.y = x, y

        if self.hud is not None:
            self.hud.text = f"P to pause   |   alive {self.elapsed:5.1f}s"

        if loom.Input.key_pressed(loom.Key.P):
            # Lay the pause menu over the level; the level stays alive below.
            self.game.scenes.push(PauseScene(), loom.Fade(0.2))


class PauseScene(loom.Scene):
    """Overlay. Resume pops back to the untouched level; Quit switches to the menu."""

    def on_enter(self):
        # The level is still drawing underneath, so a translucent panel dims it
        # rather than hiding it.
        dim = loom.Panel(loom.Color(0.0, 0.0, 0.0, 0.6))
        dim.size = loom.Vec2(0, 0)          # 0 -> fill the screen
        self.ui.add(dim)

        if self.game.font:
            title = loom.Label(self.game.font, "Paused")
            title.color = loom.Color.white()
            title.anchor = loom.Vec2(0.5, 0.3)
            title.pivot = loom.Vec2(0.5, 0.5)
            title.size = loom.Vec2(200, 30)
            title.align = loom.TextAlign.Center
            title.vcenter = True
            self.ui.add(title)

        resume = _button("Resume", self.game.font, self._resume)
        resume.anchor = loom.Vec2(0.5, 0.52)
        resume.pivot = loom.Vec2(0.5, 0.5)
        self.ui.add(resume)

        quit_ = _button("Quit to menu", self.game.font, self._quit)
        quit_.anchor = loom.Vec2(0.5, 0.52)
        quit_.pivot = loom.Vec2(0.5, 0.5)
        quit_.offset = loom.Vec2(0, 46)
        self.ui.add(quit_)

    def on_update(self, dt):
        if loom.Input.key_pressed(loom.Key.P):
            self._resume()

    def _resume(self):
        # Drop this scene; the level resumes exactly where it froze.
        self.game.scenes.pop(loom.Fade(0.2))

    def _quit(self):
        # Replace the pause menu with the menu. The level below is dropped too:
        # switch_to replaces the top of the stack, and the level is under it, so
        # pop back to the level first, then switch that out for the menu.
        self.game.scenes.pop()
        self.game.scenes.switch_to(MenuScene(), loom.Fade(0.35))


class ScenesDemo(loom.Game):
    """Holds the shared assets; the scenes hold the gameplay."""

    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit

        png = os.path.join(os.path.dirname(__file__), "_white.png")
        if not os.path.exists(png):
            _write_white_png(png, 8)
        self.tex = self.assets.texture(png)
        self.font = _load_system_font(18)

        self.frames = 0
        self.max_frames = _arg_frames()

        self.scenes.switch_to(MenuScene())
        print("Scenes demo: Play -> level, P -> pause, Resume/Quit.")

    def on_update(self, dt):
        self.frames += 1
        if self.max_frames and self.frames >= self.max_frames:
            print(f"frames={self.frames} scene={type(self.scene).__name__} "
                  f"depth={self.scenes.depth} draw_calls={self.last_draw_calls}")
            self.running = False


def _button(caption, font, on_clicked):
    b = loom.Button(font, caption) if font else loom.Button()
    b.size = loom.Vec2(180, 38)
    b.on_clicked = on_clicked
    return b


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
    loom.run(ScenesDemo(), title="loom2d — Scenes", width=960, height=540)
