"""
loom2d — Phase 2.5 Tilemap demo.

A scrollable 100x100 tile world (10,000 tiles) rendered through the batcher with
viewport culling: only the tiles inside the camera collapse into ~1 draw call,
so the map size barely matters. Move the hero with the arrow keys / WASD; the
hero collides against solid (stone) tiles using the tilemap's grid collision.

Demonstrates: Tilemap + Tileset + multiple TileLayers, grid collision, camera
follow, and culling (watch the "tiles drawn" count stay small while the map is
huge).
"""
import sys, os, math, random, zlib, struct

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

W, H = 960, 640
TILE = 32
MAP_W, MAP_H = 100, 100

# gids into our 4-tile tileset
GRASS, FLOWERS, STONE, WATER = 1, 2, 3, 4


# ── tiny solid-colour tileset PNG writer (stdlib only) ───────────────────────

def _tileset_rgba():
    """Build a 4-tile (128x32) tileset: grass, flowers, stone, water."""
    tiles = [
        (90, 160, 80),    # grass
        (120, 180, 90),   # grass + flowers (drawn below)
        (130, 130, 140),  # stone
        (70, 120, 200),   # water
    ]
    w, h = TILE * 4, TILE
    buf = bytearray(w * h * 4)
    for ti, (r, g, b) in enumerate(tiles):
        for y in range(TILE):
            for x in range(TILE):
                px = ti * TILE + x
                i = (y * w + px) * 4
                rr, gg, bb = r, g, b
                # subtle checker texture so tiles read as a grid
                if (x // 4 + y // 4) % 2 == 0:
                    rr, gg, bb = min(rr + 12, 255), min(gg + 12, 255), min(bb + 12, 255)
                # flower dots on tile 1
                if ti == 1 and (x % 10 == 5) and (y % 10 == 5):
                    rr, gg, bb = 240, 220, 80
                buf[i:i+4] = bytes((rr, gg, bb, 255))
    return w, h, bytes(buf)


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


class TilemapDemo(loom.Game):
    def on_start(self):
        d = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets')
        os.makedirs(d, exist_ok=True)
        tileset_path = os.path.join(d, 'tileset.png')
        if not os.path.exists(tileset_path):
            _write_png(tileset_path, *_tileset_rgba())
        hero_path = os.path.join(d, 'hero.png')
        if not os.path.exists(hero_path):
            _write_png(hero_path, *_hero_rgba())

        tex = self.assets.texture(tileset_path)

        # Build a 100x100 map: grass ground + a stone-walled maze of obstacles.
        self.map = loom.Tilemap(TILE, TILE, MAP_W, MAP_H)
        self.map.add_tileset(tex, TILE, TILE, first_gid=GRASS)

        ground = self.map.add_layer("ground")
        walls = self.map.add_layer("walls")

        random.seed(7)
        for ty in range(MAP_H):
            for tx in range(MAP_W):
                # scattered water ponds in the ground layer (decorative, walkable)
                ground.set(tx, ty, WATER if random.random() < 0.04 else
                           (FLOWERS if random.random() < 0.08 else GRASS))
                # solid stone: outer border + random blocks
                border = tx == 0 or ty == 0 or tx == MAP_W - 1 or ty == MAP_H - 1
                if border or random.random() < 0.10:
                    walls.set(tx, ty, STONE)

        # Carve a clear spawn area so the hero isn't stuck.
        for ty in range(2, 8):
            for tx in range(2, 8):
                walls.set(tx, ty, 0)

        self.map.set_collision_from_layer(1)  # walls layer => solid grid
        self.scene.add(self.map)

        self.hero = loom.SpriteNode(self.assets.texture(hero_path))
        self.hero.origin = loom.Vec2(0.5, 0.5)
        self.hero.position = loom.Vec2(4.5 * TILE, 4.5 * TILE)
        self.scene.add(self.hero)

        self.speed = 220.0
        self._title_timer = 0.0
        print(f"Tilemap demo: {MAP_W}x{MAP_H} = {MAP_W*MAP_H} tiles, 2 layers.")
        print("Move with arrows / WASD. Watch culling keep 'tiles drawn' small.")

    def _blocked(self, cx, cy):
        # Hero AABB (24x24) centred at (cx, cy); query the tilemap's solid grid.
        r = loom.Rect(cx - 12, cy - 12, 24, 24)
        return self.map.rect_overlaps_solid(r)

    def on_update(self, dt):
        dx = dy = 0.0
        if loom.Input.key_down(loom.Key.Left)  or loom.Input.key_down(loom.Key.A): dx -= 1
        if loom.Input.key_down(loom.Key.Right) or loom.Input.key_down(loom.Key.D): dx += 1
        if loom.Input.key_down(loom.Key.Up)    or loom.Input.key_down(loom.Key.W): dy -= 1
        if loom.Input.key_down(loom.Key.Down)  or loom.Input.key_down(loom.Key.S): dy += 1
        if dx and dy:
            inv = 1.0 / math.sqrt(2.0)
            dx *= inv; dy *= inv

        # Axis-separated movement so we slide along walls instead of sticking.
        nx = self.hero.x + dx * self.speed * dt
        if not self._blocked(nx, self.hero.y):
            self.hero.x = nx
        ny = self.hero.y + dy * self.speed * dt
        if not self._blocked(self.hero.x, ny):
            self.hero.y = ny

        if loom.Input.key_pressed(loom.Key.Escape):
            self.running = False

        # Camera follows the hero.
        self.scene.camera.position = self.hero.position

        self._title_timer += dt
        if self._title_timer > 0.5:
            self._title_timer = 0.0
            print(f"tiles drawn: {self.map.tiles_drawn:4d} / {MAP_W*MAP_H}  "
                  f"draw calls: {self.last_draw_calls}", end='\r')


def _hero_rgba():
    size = 24
    half = size / 2.0
    buf = bytearray()
    for y in range(size):
        for x in range(size):
            dx, dy = x - (half - 0.5), y - (half - 0.5)
            if math.sqrt(dx * dx + dy * dy) < half - 1.0:
                buf.extend([230, 70, 70, 255])
            else:
                buf.extend([0, 0, 0, 0])
    return size, size, bytes(buf)


if __name__ == '__main__':
    loom.run(TilemapDemo(), title="loom2d — Tilemap Demo", width=W, height=H)
