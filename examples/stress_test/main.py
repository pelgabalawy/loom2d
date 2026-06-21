"""
loom2d — Phase 2 GPU batcher stress test.

Draws thousands of sprites that all share one texture. With the sokol_gfx
SpriteBatcher they collapse into a single draw call, so this should hold 60fps
(vsync-capped) even at 10,000 sprites — the acceptance gate for Phase 2.

Runs a fixed number of frames, prints the measured FPS + draw-call count, then
exits, so it can be used as an automated benchmark.
"""
import sys, os, zlib, struct, math, random, time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

W, H = 1280, 720
N_SPRITES = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
STATIC = 'static' in sys.argv  # skip the per-sprite Python movement loop
WARMUP_FRAMES  = 30
MEASURE_FRAMES = 240


# ── tiny PNG writer (stdlib only) ────────────────────────────────────────────

def _circle_rgba(size, r, g, b):
    half = size / 2.0
    buf = bytearray()
    for y in range(size):
        for x in range(size):
            dx, dy = x - (half - 0.5), y - (half - 0.5)
            dist = math.sqrt(dx * dx + dy * dy)
            if dist < half - 1.0:
                buf.extend([r, g, b, 255])
            elif dist < half:
                buf.extend([r, g, b, int((half - dist) * 255)])
            else:
                buf.extend([0, 0, 0, 0])
    return bytes(buf)


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


class StressTest(loom.Game):
    def on_start(self):
        d = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets')
        os.makedirs(d, exist_ok=True)
        p = os.path.join(d, 'blob.png')
        if not os.path.exists(p):
            _write_png(p, 16, 16, _circle_rgba(16, 90, 200, 255))
        tex = self.assets.texture(p)

        self.sprites = []
        for _ in range(N_SPRITES):
            s = loom.SpriteNode(tex)
            s.origin = loom.Vec2(0.5, 0.5)
            s.position = loom.Vec2(random.uniform(0, W), random.uniform(0, H))
            s.tint = loom.Color(random.random(), random.random(), random.random(), 1.0)
            self.scene.add(s)
            self.sprites.append([s, random.uniform(-60, 60), random.uniform(-60, 60)])

        self.frame = 0
        self.elapsed = 0.0
        self.worst = 0.0
        self.t_last = None
        print(f"Stress test: {N_SPRITES} sprites, one shared texture.")

    def on_update(self, dt):
        # Measure true wall-clock frame time (the engine clamps dt, so we can't
        # use it for FPS). This is the full frame period: update + draw + swap.
        now = time.perf_counter()
        if self.t_last is not None and self.frame > WARMUP_FRAMES:
            frame_dt = now - self.t_last
            self.elapsed += frame_dt
            self.worst = max(self.worst, frame_dt)
        self.t_last = now

        # Keep things moving so we're not just drawing static geometry.
        # Use float x/y props (no Vec2 allocation) to minimise Python overhead.
        for entry in (() if STATIC else self.sprites):
            s, vx, vy = entry
            nx, ny = s.x + vx * dt, s.y + vy * dt
            if nx < 0 or nx > W: entry[1] = -vx
            if ny < 0 or ny > H: entry[2] = -vy
            s.x = nx
            s.y = ny

        self.frame += 1
        if self.frame >= WARMUP_FRAMES + MEASURE_FRAMES:
            measured = self.frame - 1 - WARMUP_FRAMES
            avg = measured / self.elapsed if self.elapsed > 0 else 0.0
            print(f"Avg FPS: {avg:.1f}  (worst frame {self.worst*1000:.1f} ms)")
            print(f"Draw calls/frame: {self.last_draw_calls}  "
                  f"for {N_SPRITES} sprites")
            self.running = False


if __name__ == '__main__':
    loom.run(StressTest(), title="loom2d — Batcher Stress Test", width=W, height=H)
