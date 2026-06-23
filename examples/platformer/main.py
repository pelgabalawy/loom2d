"""
loom2d platformer — a small, *playable* game built on the physics engine.

Run it:
    python examples/platformer/main.py
    python examples/platformer/main.py --frames 600   # self-terminating demo

Controls:  A / D  (or arrow keys) to move,  Space to jump,  Escape to quit.

Shows off the physics engine and the Phase 2.65 events:
  - dynamic player body + static ground/platforms (Box2D)
  - contact events drive event-accurate jumping (you can only jump while you
    are actually standing on something — no mid-air double jumps)
  - coins are sensor bodies: they detect the player and vanish without ever
    blocking movement
  - body tags identify who-touched-what
  - sprites are synced to physics bodies every frame; a TextNode HUD shows score
"""
import sys, os, zlib, struct, math

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "python"))
import loom2d as loom

W, H = 800, 600
PLAYER_SPEED = 220.0   # px/s
JUMP_IMPULSE = 430.0   # px/s upward impulse

FRAMES = None
if "--frames" in sys.argv:
    FRAMES = int(sys.argv[sys.argv.index("--frames") + 1])


# ── tiny PNG writer (stdlib only, no Pillow) ─────────────────────────────────

def _write_png(path, w, h, rgba):
    def chunk(tag, data):
        crc = zlib.crc32(tag + data) & 0xffffffff
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)
    rows = b"".join(b"\x00" + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n"
                + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(rows, 9))
                + chunk(b"IEND", b""))


def _solid(w, h, r, g, b, a=255):
    return bytes([r, g, b, a]) * (w * h)


def _coin(size=24):
    half = size / 2.0
    buf = bytearray()
    for y in range(size):
        for x in range(size):
            dx, dy = x - (half - 0.5), y - (half - 0.5)
            d = math.sqrt(dx * dx + dy * dy)
            if d < half - 1:
                t = 1.0 - d / half
                buf += bytes((255, min(255, int(200 + 55 * t)), 40, 255))
            elif d < half:
                buf += bytes((255, 200, 40, int((half - d) * 255)))
            else:
                buf += bytes((0, 0, 0, 0))
    return bytes(buf)


def gen_assets(d):
    os.makedirs(d, exist_ok=True)
    if not os.path.exists(os.path.join(d, "player.png")):
        _write_png(os.path.join(d, "player.png"), 1, 1, _solid(1, 1, 90, 170, 255))
    if not os.path.exists(os.path.join(d, "ground.png")):
        _write_png(os.path.join(d, "ground.png"), 1, 1, _solid(1, 1, 110, 80, 50))
    if not os.path.exists(os.path.join(d, "platform.png")):
        _write_png(os.path.join(d, "platform.png"), 1, 1, _solid(1, 1, 90, 130, 90))
    if not os.path.exists(os.path.join(d, "coin.png")):
        _write_png(os.path.join(d, "coin.png"), 24, 24, _coin(24))


def _find_system_font():
    for p in (r"C:\Windows\Fonts\arial.ttf", r"C:\Windows\Fonts\segoeui.ttf",
              "/System/Library/Fonts/Supplemental/Arial.ttf",
              "/Library/Fonts/Arial.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"):
        if os.path.exists(p):
            return p
    return None


# Level layout: floating platforms as (center_x, center_y, half_w, half_h)
PLATFORMS = [
    (160, 440, 90, 12),
    (400, 360, 90, 12),
    (640, 460, 90, 12),
    (560, 250, 70, 12),
]
# Coins sit just above a platform (or the ground).
COINS = [(160, 410), (400, 330), (640, 430), (560, 220), (300, 540)]

PLAYER_HW, PLAYER_HH = 14, 22  # half-extents (sprite is 28 x 44)


class Platformer(loom.Game):
    def on_start(self):
        self.clear_color = loom.Color(0.10, 0.12, 0.20)
        d = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets")
        gen_assets(d)

        tex_player   = self.assets.texture(os.path.join(d, "player.png"))
        tex_ground   = self.assets.texture(os.path.join(d, "ground.png"))
        tex_platform = self.assets.texture(os.path.join(d, "platform.png"))
        tex_coin     = self.assets.texture(os.path.join(d, "coin.png"))

        self.score = 0
        self.ground_contacts = 0     # how many solid surfaces the player touches
        self.start_pos = loom.Vec2(W / 2, 120)

        # Ground (static body + a wide sprite strip)
        ground = self.physics.create_body(loom.BodyType.Static, loom.Vec2(W / 2, H - 16))
        ground.add_box(W / 2, 16)
        ground.tag = "solid"
        gs = loom.SpriteNode(tex_ground)
        gs.origin = loom.Vec2(0.5, 0.5)
        gs.position = loom.Vec2(W / 2, H - 16)
        gs.scale = loom.Vec2(W, 32)
        self.scene.add(gs)

        # Floating platforms
        for (cx, cy, hw, hh) in PLATFORMS:
            body = self.physics.create_body(loom.BodyType.Static, loom.Vec2(cx, cy))
            body.add_box(hw, hh)
            body.tag = "solid"
            s = loom.SpriteNode(tex_platform)
            s.origin = loom.Vec2(0.5, 0.5)
            s.position = loom.Vec2(cx, cy)
            s.scale = loom.Vec2(hw * 2, hh * 2)
            self.scene.add(s)

        # Coins — sensor bodies that vanish on pickup
        self.coins = []   # list of (body, sprite)
        for (cx, cy) in COINS:
            body = self.physics.create_body(loom.BodyType.Static, loom.Vec2(cx, cy))
            body.add_box(12, 12, is_sensor=True)
            body.tag = "coin"
            s = loom.SpriteNode(tex_coin)
            s.origin = loom.Vec2(0.5, 0.5)
            s.position = loom.Vec2(cx, cy)
            self.scene.add(s)
            self.coins.append((body, s))
        self.total = len(self.coins)

        # Player (dynamic body + sprite)
        self.player = self.physics.create_body(loom.BodyType.Dynamic, self.start_pos)
        self.player.add_box(PLAYER_HW, PLAYER_HH, density=1.0, friction=0.2)
        self.player.tag = "player"
        self.player_node = loom.SpriteNode(tex_player)
        self.player_node.origin = loom.Vec2(0.5, 0.5)
        self.player_node.scale = loom.Vec2(PLAYER_HW * 2, PLAYER_HH * 2)
        self.scene.add(self.player_node)

        # HUD
        self.hud = None
        fp = _find_system_font()
        if fp:
            self.hud = loom.TextNode(loom.Font.load(fp, 26), "")
            self.hud.color = loom.Color(1.0, 0.95, 0.6, 1.0)
            self.scene.add(self.hud)

        # Physics event wiring
        self.physics.on_contact_begin = self._on_contact_begin
        self.physics.on_contact_end   = self._on_contact_end
        self.physics.on_sensor_begin  = self._on_sensor_begin

        print("Platformer — A/D or arrows to move, Space to jump, Esc to quit.")
        print(f"Collect all {self.total} coins!")

    # ── physics events ───────────────────────────────────────────────────────
    def _is(self, a, b, t1, t2):
        return {a.tag, b.tag} == {t1, t2}

    def _on_contact_begin(self, a, b):
        if self._is(a, b, "player", "solid"):
            self.ground_contacts += 1

    def _on_contact_end(self, a, b):
        if self._is(a, b, "player", "solid"):
            self.ground_contacts = max(0, self.ground_contacts - 1)

    def _on_sensor_begin(self, sensor, visitor):
        if sensor.tag == "coin" and visitor.tag == "player":
            for i, (body, sprite) in enumerate(self.coins):
                if body is sensor:
                    sprite.remove_from_parent()
                    self.physics.destroy_body(body)
                    self.coins.pop(i)
                    self.score += 1
                    print(f"coin! {self.score}/{self.total}")
                    break

    # ── main loop ──────────────────────────────────────────────────────────────
    def on_update(self, dt):
        on_ground = self.ground_contacts > 0

        # Input (in --frames demo mode, auto-walk right and hop)
        left = loom.Input.key_down(loom.Key.Left) or loom.Input.key_down(loom.Key.A)
        right = loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.D)
        jump = loom.Input.key_pressed(loom.Key.Space)
        if FRAMES is not None:
            right = True
            jump = on_ground and (int(self.elapsed_frames() ) % 45 == 0)

        vx = (PLAYER_SPEED if right else 0.0) - (PLAYER_SPEED if left else 0.0)
        vy = self.player.linear_velocity.y
        self.player.set_linear_velocity(loom.Vec2(vx, vy))

        if jump and on_ground:
            self.player.apply_impulse(loom.Vec2(0, -JUMP_IMPULSE))
            self.ground_contacts = 0

        # Respawn if the player falls out of the world
        if self.player.position.y > H + 80:
            self.player.set_position(self.start_pos)
            self.player.set_linear_velocity(loom.Vec2(0, 0))

        # Sync sprite to body
        p = self.player.position
        self.player_node.position = loom.Vec2(p.x, p.y)

        # HUD pinned to top-left of the (static) view
        if self.hud is not None:
            self.hud.position = self.scene.camera.screen_to_world(loom.Vec2(12, 12))
            label = f"coins {self.score}/{self.total}"
            if self.score == self.total:
                label += "   -   YOU WIN!"
            self.hud.text = label

        if loom.Input.key_pressed(loom.Key.Escape):
            self.running = False

        if FRAMES is not None:
            self._frame = getattr(self, "_frame", 0) + 1
            if self._frame % 60 == 0 or self._frame >= FRAMES:
                print(f"frame {self._frame:4d}  score {self.score}/{self.total}  "
                      f"draw_calls {self.last_draw_calls}")
            if self._frame >= FRAMES:
                self.running = False

    def elapsed_frames(self):
        self._frame = getattr(self, "_frame", 0)
        return self._frame

    def on_stop(self):
        print(f"Final score: {self.score}/{self.total}")


if __name__ == "__main__":
    loom.run(Platformer(), title="loom2d — Platformer", width=W, height=H)
