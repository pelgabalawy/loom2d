"""
coin_quest — a tiny top-down game that exercises Phase 2.5 end to end.

Navigate a 40x30 tile maze, collect all the coins. Demonstrates, together:
  * Tilemap rendering (ground + walls layers) through the batcher
  * Viewport culling (watch "tiles drawn" stay small as you roam)
  * Grid collision (the hero can't walk through stone)
  * Camera follow + sprite batching (hero + coins share the draw path)

Run interactively:   python main.py
Self-terminating test (auto-pilots to collect coins, prints diagnostics,
then exits — usable on any machine with a display as a render smoke test):
    python main.py --frames 600
"""
import os, sys, math, zlib, struct

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom
import world as W

WIN_W, WIN_H = 960, 640

FRAMES = None
if '--frames' in sys.argv:
    FRAMES = int(sys.argv[sys.argv.index('--frames') + 1])


# ── tiny PNG helpers (stdlib only) ───────────────────────────────────────────

def _write_png(path, w, h, rgba):
    def chunk(tag, data):
        crc = zlib.crc32(tag + data) & 0xffffffff
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', crc)
    rows = b''.join(b'\x00' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n'
                + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                + chunk(b'IDAT', zlib.compress(rows, 9))
                + chunk(b'IEND', b''))


def _solid_tile(r, g, b):
    out = bytearray()
    for y in range(W.TILE):
        for x in range(W.TILE):
            checker = (x // 4 + y // 4) % 2 == 0
            d = 12 if checker else 0
            out += bytes((min(r + d, 255), min(g + d, 255), min(b + d, 255), 255))
    return bytes(out)


def _tileset_png(path):
    # Two tiles side by side: grass (gid 1), stone (gid 2).
    w, h = W.TILE * 2, W.TILE
    grass, stone = _solid_tile(90, 160, 80), _solid_tile(130, 130, 140)
    buf = bytearray(w * h * 4)
    for y in range(W.TILE):
        row = y * w * 4
        buf[row:row + W.TILE * 4] = grass[y * W.TILE * 4:(y + 1) * W.TILE * 4]
        buf[row + W.TILE * 4:row + w * 4] = stone[y * W.TILE * 4:(y + 1) * W.TILE * 4]
    _write_png(path, w, h, bytes(buf))


def _find_system_font():
    """Best-effort path to a TTF for the on-screen HUD (None if none found)."""
    for p in (r"C:\Windows\Fonts\arial.ttf", r"C:\Windows\Fonts\segoeui.ttf",
              "/System/Library/Fonts/Supplemental/Arial.ttf",
              "/Library/Fonts/Arial.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"):
        if os.path.exists(p):
            return p
    return None


def _disc_png(path, size, color):
    half = size / 2.0
    out = bytearray()
    for y in range(size):
        for x in range(size):
            dx, dy = x - (half - 0.5), y - (half - 0.5)
            out += bytes((*color, 255)) if math.hypot(dx, dy) < half - 1.0 \
                else bytes((0, 0, 0, 0))
    _write_png(path, size, size, bytes(out))


class CoinQuest(loom.Game):
    def on_start(self):
        d = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets')
        os.makedirs(d, exist_ok=True)
        tileset_p = os.path.join(d, 'tileset.png')
        hero_p    = os.path.join(d, 'hero.png')
        coin_p    = os.path.join(d, 'coin.png')
        if not os.path.exists(tileset_p): _tileset_png(tileset_p)
        if not os.path.exists(hero_p):    _disc_png(hero_p, W.HERO_SIZE, (230, 70, 70))
        if not os.path.exists(coin_p):    _disc_png(coin_p, 16, (245, 210, 60))

        self.map = W.build_map()
        self.map.add_tileset(self.assets.texture(tileset_p), W.TILE, W.TILE, first_gid=W.GRASS)
        self.scene.add(self.map)

        coin_tex = self.assets.texture(coin_p)
        self.coins = W.place_coins(self.map)
        self.coin_nodes = []
        for c in self.coins:
            s = loom.SpriteNode(coin_tex)
            s.origin = loom.Vec2(0.5, 0.5)
            s.position = c
            self.scene.add(s)
            self.coin_nodes.append(s)

        self.hero = loom.SpriteNode(self.assets.texture(hero_p))
        self.hero.origin = loom.Vec2(0.5, 0.5)
        self.hero.position = W.spawn_position()
        self.scene.add(self.hero)

        self.speed = 200.0
        self.score = 0
        self.total = len(self.coins)
        self.frame = 0
        self.won = False

        # On-screen score HUD (pinned to the top-left of the view each frame so
        # it stays put while the camera follows the hero). Skipped gracefully if
        # no system font is available.
        self.hud = None
        font_path = _find_system_font()
        if font_path:
            self.hud = loom.TextNode(loom.Font.load(font_path, 28),
                                     f"coins 0/{self.total}")
            self.hud.color = loom.Color(1.0, 0.95, 0.6, 1.0)
            self.scene.add(self.hud)
        print(f"coin_quest: {W.MAP_W}x{W.MAP_H} map, {self.total} coins. "
              f"{'AUTO-PILOT' if FRAMES else 'Arrows/WASD to move, Esc to quit.'}")

    def _input_dir(self):
        if FRAMES is not None:
            # Auto-pilot: steer toward the nearest remaining coin (greedy).
            if not self.coins:
                return 0.0, 0.0
            hx, hy = self.hero.x, self.hero.y
            target = min(self.coins, key=lambda c: (c.x - hx) ** 2 + (c.y - hy) ** 2)
            return target.x - hx, target.y - hy
        dx = dy = 0.0
        if loom.Input.key_down(loom.Key.Left)  or loom.Input.key_down(loom.Key.A): dx -= 1
        if loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.D): dx += 1
        if loom.Input.key_down(loom.Key.Up)    or loom.Input.key_down(loom.Key.W): dy -= 1
        if loom.Input.key_down(loom.Key.Down)  or loom.Input.key_down(loom.Key.S): dy += 1
        return dx, dy

    def on_update(self, dt):
        self.frame += 1
        dx, dy = self._input_dir()
        mag = math.hypot(dx, dy)
        if mag > 0:
            step = self.speed * dt
            pos = W.try_move(self.map, self.hero.position,
                             dx / mag * step, dy / mag * step)
            self.hero.position = pos

        # Collect coins under the hero; despawn their sprites.
        self.coins, collected = W.collect(self.hero.position, self.coins)
        for i in sorted(collected, reverse=True):
            self.coin_nodes.pop(i).remove_from_parent()
            self.score += 1

        self.scene.camera.position = self.hero.position

        # Keep the HUD pinned to the view's top-left corner (10px inset).
        if self.hud is not None:
            self.hud.position = self.scene.camera.screen_to_world(loom.Vec2(10, 10))
            label = f"coins {self.score}/{self.total}"
            if self.won:
                label += "  -  YOU WIN!"
            self.hud.text = label

        if not self.coins and not self.won:
            self.won = True
            print(f"\nAll {self.total} coins collected in {self.frame} frames — you win!")
            if FRAMES is None:
                self.running = False

        if loom.Input.key_pressed(loom.Key.Escape):
            self.running = False

        if FRAMES is not None:
            if self.frame % 60 == 0 or self.frame == FRAMES:
                print(f"frame {self.frame:4d}  score {self.score}/{self.total}  "
                      f"tiles_drawn {self.map.tiles_drawn:3d}/{W.MAP_W*W.MAP_H}  "
                      f"draw_calls {self.last_draw_calls}")
            if self.frame >= FRAMES or self.won:
                self.running = False


if __name__ == '__main__':
    loom.run(CoinQuest(), title="loom2d — coin_quest", width=WIN_W, height=WIN_H)
