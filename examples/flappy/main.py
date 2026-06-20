"""
Flappy Bird — built with nothing but loom2d.

Architecture: each game element is its own class (component-style).
  Bird        — physics, input, tilt visual
  PipePair    — top + bottom pipe sprites, scrolling, collision
  PipeSpawner — timer, randomised gap placement, cleanup
  Ground      — static visual strip at the bottom
"""
import sys, os, zlib, struct, math, time, random

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

# ── Tuning constants ─────────────────────────────────────────────────────────

W, H = 480, 640

BIRD_X     = 110
BIRD_SIZE  = 36
BIRD_HALF  = BIRD_SIZE / 2.0
GRAVITY    = 1400.0   # px/s²
JUMP_VY    = -490.0   # px/s  (up = negative)
MAX_FALL   = 700.0

PIPE_W     = 72
PIPE_GAP   = 160      # open gap between top and bottom pipe
PIPE_SPEED = 210.0    # px/s leftward
PIPE_EVERY = 2.2      # seconds between spawns
PIPE_MIN_H = 90       # minimum pipe stub height (top or bottom)

GROUND_H   = 68
PLAY_H     = H - GROUND_H   # usable play area height

WAITING, PLAYING, DEAD = 0, 1, 2

# ── PNG generation (stdlib only) ─────────────────────────────────────────────

def _circle_rgba(size, r, g, b):
    half = size / 2.0
    buf = bytearray()
    for y in range(size):
        for x in range(size):
            dx, dy = x - (half - 0.5), y - (half - 0.5)
            dist = math.sqrt(dx * dx + dy * dy)
            if dist < half - 1.0:
                t = 1.0 - dist / half
                rr = min(255, int(r + (255 - r) * t * 0.28))
                gg = min(255, int(g + (255 - g) * t * 0.18))
                buf.extend([rr, gg, b, 255])
            elif dist < half:
                buf.extend([r, g, b, int((half - dist) * 255)])
            else:
                buf.extend([0, 0, 0, 0])
    return bytes(buf)


def _solid_rgba(w, h, r, g, b, a=255):
    return bytes([r, g, b, a]) * (w * h)


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


def gen_assets(d):
    os.makedirs(d, exist_ok=True)
    specs = {
        'bird.png':   (_circle_rgba, (32, 32, 255, 215, 0)),   # golden circle
        'pipe.png':   (_solid_rgba,  ( 1,  1,  45, 150, 45)),  # green block
        'ground.png': (_solid_rgba,  ( 1,  1, 120,  90, 30)),  # earth brown
    }
    for name, (fn, args) in specs.items():
        p = os.path.join(d, name)
        if not os.path.exists(p):
            w, h, *rgb = args
            _write_png(p, w, h, fn(w, h, *rgb))

# ── Components ───────────────────────────────────────────────────────────────

class Bird:
    """Handles the bird's physics, input response, and tilt visual."""

    BOB_SPEED   = 3.0   # radians/s for idle bob
    BOB_RANGE   = 9.0   # pixels

    def __init__(self, scene, texture):
        s = BIRD_SIZE / 32.0
        self._sprite = loom.SpriteNode(texture)
        self._sprite.origin = loom.Vec2(0.5, 0.5)
        self._sprite.scale  = loom.Vec2(s, s)
        scene.add(self._sprite)
        self.reset()

    def reset(self):
        self._vy = 0.0
        self.y   = H / 2.0 - 40.0
        self._sprite.rotation = 0.0
        self._sprite.position = loom.Vec2(BIRD_X, self.y)

    def flap(self):
        self._vy = JUMP_VY

    def update(self, dt):
        self._vy = min(self._vy + GRAVITY * dt, MAX_FALL)
        self.y  += self._vy * dt
        tilt_deg = max(-30.0, min(25.0, self._vy * 0.04))
        self._sprite.rotation = math.radians(tilt_deg)
        self._sprite.position = loom.Vec2(BIRD_X, self.y)

    def bob(self):
        """Idle bob animation while waiting to start."""
        self.y = H / 2.0 - 40.0 + math.sin(time.time() * self.BOB_SPEED) * self.BOB_RANGE
        self._sprite.position = loom.Vec2(BIRD_X, self.y)

    def die_fall(self, dt):
        """Continue falling after death."""
        self._vy = min(self._vy + GRAVITY * dt, MAX_FALL)
        self.y   = min(self.y + self._vy * dt, PLAY_H - BIRD_HALF)
        self._sprite.rotation = math.radians(90)
        self._sprite.position = loom.Vec2(BIRD_X, self.y)

    def hit_ground(self):
        return self.y + BIRD_HALF >= PLAY_H

    def hit_ceiling(self):
        return self.y - BIRD_HALF <= 0

    @property
    def hitbox(self):
        """Slightly shrunken AABB for fairer feel."""
        m = 6
        return (BIRD_X - BIRD_HALF + m, self.y - BIRD_HALF + m,
                BIRD_X + BIRD_HALF - m, self.y + BIRD_HALF - m)


class PipePair:
    """One pair of pipes (top hanging + bottom rising) at a given x position."""

    def __init__(self, scene, texture, x, gap_top):
        gap_bot = gap_top + PIPE_GAP
        self.x       = float(x)
        self.gap_top = gap_top
        self.gap_bot = gap_bot
        self.scored  = False

        self._top = loom.SpriteNode(texture)
        self._top.origin   = loom.Vec2(0, 0)
        self._top.position = loom.Vec2(x, 0)
        self._top.scale    = loom.Vec2(PIPE_W, max(1, gap_top))
        scene.add(self._top)

        bot_h = PLAY_H - gap_bot
        self._bot = loom.SpriteNode(texture)
        self._bot.origin   = loom.Vec2(0, 0)
        self._bot.position = loom.Vec2(x, gap_bot)
        self._bot.scale    = loom.Vec2(PIPE_W, max(1, bot_h))
        scene.add(self._bot)

    def update(self, dt):
        self.x    -= PIPE_SPEED * dt
        self._top.x = self.x
        self._bot.x = self.x

    def collides_with(self, bx1, by1, bx2, by2):
        if bx2 > self.x and bx1 < self.x + PIPE_W:
            return by1 < self.gap_top or by2 > self.gap_bot
        return False

    def is_off_screen(self):
        return self.x + PIPE_W < -20

    def remove(self):
        self._top.remove_from_parent()
        self._bot.remove_from_parent()


class PipeSpawner:
    """Generates pipe pairs at regular intervals with random gap positions."""

    def __init__(self, scene, texture):
        self._scene   = scene
        self._texture = texture
        self.pipes    = []
        self._timer   = 1.5   # delay before first pipe

    def reset(self):
        for p in self.pipes:
            p.remove()
        self.pipes  = []
        self._timer = 1.5

    def update(self, dt):
        self._timer -= dt
        if self._timer <= 0:
            self._spawn()
            self._timer = PIPE_EVERY

        dead = []
        for pipe in self.pipes:
            pipe.update(dt)
            if pipe.is_off_screen():
                dead.append(pipe)

        for p in dead:
            p.remove()
            self.pipes.remove(p)

    def _spawn(self):
        gap_top = random.randint(PIPE_MIN_H, PLAY_H - PIPE_GAP - PIPE_MIN_H)
        self.pipes.append(PipePair(self._scene, self._texture, W + 20, gap_top))


class Ground:
    """Static earth strip at the bottom of the screen."""

    def __init__(self, scene, texture):
        sprite = loom.SpriteNode(texture)
        sprite.origin   = loom.Vec2(0, 0)
        sprite.position = loom.Vec2(0, PLAY_H)
        sprite.scale    = loom.Vec2(W, GROUND_H)
        scene.add(sprite)

# ── Game ─────────────────────────────────────────────────────────────────────

class FlappyBird(loom.Game):

    def on_start(self):
        self.auto_physics = False
        self.clear_color  = loom.Color(135/255, 206/255, 235/255)   # sky blue

        d = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'assets')
        gen_assets(d)

        tex_bird   = self.assets.texture(os.path.join(d, 'bird.png'))
        tex_pipe   = self.assets.texture(os.path.join(d, 'pipe.png'))
        tex_ground = self.assets.texture(os.path.join(d, 'ground.png'))

        self._ground  = Ground(self.scene, tex_ground)
        self._bird    = Bird(self.scene, tex_bird)
        self._spawner = PipeSpawner(self.scene, tex_pipe)

        self._state = WAITING
        self._score = 0

        print("=== Flappy Bird (loom2d) ===")
        print("  SPACE or left-click to flap | SPACE after death to restart")

    def on_update(self, dt):
        jumped = (loom.Input.key_pressed(loom.Key.Space)
                  or loom.Input.mouse_pressed(loom.MouseButton.Left))

        if self._state == WAITING:
            self._bird.bob()
            if jumped:
                self._state = PLAYING
                self._bird.flap()
            return

        if self._state == DEAD:
            self._bird.die_fall(dt)
            if jumped:
                self._restart()
            return

        # ── Playing ──────────────────────────────────────────────────────────

        if jumped:
            self._bird.flap()

        self._bird.update(dt)
        self._spawner.update(dt)

        # Ground / ceiling
        if self._bird.hit_ground() or self._bird.hit_ceiling():
            self._die()
            return

        # Pipe collision + scoring
        bx1, by1, bx2, by2 = self._bird.hitbox
        for pipe in self._spawner.pipes:
            if pipe.collides_with(bx1, by1, bx2, by2):
                self._die()
                return
            if not pipe.scored and pipe.x + PIPE_W < BIRD_X:
                pipe.scored = True
                self._score += 1
                print(f"  Score: {self._score}")

    def _die(self):
        self._state = DEAD
        print(f"  Game over! Score: {self._score}  -- SPACE to restart")

    def _restart(self):
        self._bird.reset()
        self._spawner.reset()
        self._state = WAITING
        self._score = 0

    def on_stop(self):
        print(f"Thanks for playing! Final score: {self._score}")


loom.run(FlappyBird(), title="Flappy Bird - loom2d", width=W, height=H)
