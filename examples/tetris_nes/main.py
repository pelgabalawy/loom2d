"""
NES Tetris — a faithful clone built with loom2d.

Run:            python examples/tetris_nes/main.py
Start on lvl 9: python examples/tetris_nes/main.py --level 9
Smoke test:     python examples/tetris_nes/main.py --frames 300

Controls
  Left / Right ....... move (16f DAS charge, 6f auto-repeat, like the NES)
  Down ............... soft drop (+1 point per row; the NES has no hard drop)
  X / Up ............. rotate clockwise      (NES 'A' button)
  Z / Ctrl ........... rotate anticlockwise  (NES 'B' button)
  Enter .............. confirm menu / pause / restart after a top-out
  Left/Right in menu . choose starting level (0-9); Up/Down jumps by 5
  M / S .............. mute music / mute sound effects (independently)
  Escape ............. quit

Gamepad (hot-pluggable, mapped like the NES pad)
  D-pad / left stick . move and soft-drop
  A or B ............. rotate clockwise      (the NES 'A' button)
  X or Y ............. rotate anticlockwise  (the NES 'B' button)
  Start .............. confirm / pause / restart
  A rumble fires on a Tetris and on a top-out.

  The MUSIC and SFX buttons in the bottom-left corner toggle the same two
  things with the mouse — they are real loom2d UI widgets on the screen-space
  canvas, so the world camera never moves them.

Design
  tetris_core.py holds the rules engine (Board / Piece / TetrisEngine / ...) with
  no loom2d dependency. This file is the presentation layer, one class per
  concern:

    AssetForge   generates the textures and audio (stdlib only, no art assets)
    Palette      NES per-level piece colours
    BlockPool    a reusable pool of bevelled block sprites
    WellView     draws the playfield + the falling piece
    NextBoxView  draws the next-piece preview
    StatsView    draws the statistics panel
    HudView      draws LINES / TOP / SCORE / LEVEL
    Overlays     title, level select, PAUSE and GAME OVER text
    SoundBank    maps engine Events to sounds; music and SFX mute separately
    SoundPanel   the MUSIC / SFX on-off buttons (loom2d UI widgets)
    HighScore    persists the top score between runs
    TetrisGame   the loom.Game: owns the screens and the fixed 60 Hz clock

The engine is stepped on a fixed 60 Hz accumulator, so NES frame counts (gravity,
DAS, ARE) are exact regardless of the monitor's refresh rate.
"""
import sys, os, struct, zlib, wave

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

from tetris_core import (TetrisEngine, Tetromino, Inputs, Event, State, Rules,
                         LineClear)

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, 'assets')

# ── Layout (pixels; +y is down) ──────────────────────────────────────────────
CELL = 24                       # playfield cell size
MINI = 14                       # cell size in the next box + statistics
BLOCK_TEX = 8                   # native pixel size of the block texture

BX, BY = 208, 96                # top-left of the well
FIELD_W, FIELD_H = Rules.COLS * CELL, Rules.ROWS * CELL
LOGICAL_W, LOGICAL_H = 640, 600
FRAME = 1.0 / 60.0

BASE_BG = loom.Color(0.04, 0.04, 0.06, 1.0)     # the screen behind everything
FIELD_BG = loom.Color(0.0, 0.0, 0.0, 1.0)       # the well's black backdrop


class Palette:
    """NES per-level piece colours: two level colours plus white.

    The whole field recolours on a level-up, exactly like the NES, because every
    block's tint is looked up from the *current* level each frame.
    """

    LEVELS = [
        ((0x00, 0x58, 0xF8), (0x3C, 0xBC, 0xFC)),   # 0  blue / cyan
        ((0x00, 0xA8, 0x00), (0xB8, 0xF8, 0x18)),   # 1  green / lime
        ((0xD8, 0x00, 0xCC), (0xF8, 0x78, 0xF8)),   # 2  magenta / pink
        ((0x00, 0x58, 0xF8), (0x58, 0xD8, 0x54)),   # 3  blue / green
        ((0xE4, 0x00, 0x58), (0x58, 0xF8, 0x98)),   # 4  red / mint
        ((0x6C, 0x00, 0xE0), (0x00, 0xE8, 0xD8)),   # 5  purple / teal
        ((0xF8, 0x38, 0x00), (0x88, 0x88, 0x88)),   # 6  orange / grey
        ((0x88, 0x00, 0xF8), (0xE4, 0x00, 0x58)),   # 7  violet / red
        ((0x00, 0x58, 0xF8), (0xE4, 0x00, 0x58)),   # 8  blue / red
        ((0xF8, 0x38, 0x00), (0xF8, 0xB8, 0x00)),   # 9  orange / gold
    ]
    WHITE = (0xFC, 0xFC, 0xFC)

    # A colour is looked up per cell per frame, so build each one once.
    _cache = {}

    @staticmethod
    def rgb(rgb, alpha=1.0):
        r, g, b = rgb
        return loom.Color(r / 255.0, g / 255.0, b / 255.0, alpha)

    @classmethod
    def for_slot(cls, level, color_slot):
        key = (level % 10, color_slot)
        color = cls._cache.get(key)
        if color is None:
            source = cls.WHITE if color_slot == 2 else cls.LEVELS[key[0]][color_slot]
            color = cls.rgb(source)
            cls._cache[key] = color
        return color

    @classmethod
    def for_shape(cls, level, shape):
        return cls.for_slot(level, shape.color_slot)

    @classmethod
    def for_cell(cls, level, cell_value):
        """cell_value is a Board cell: tetromino index + 1."""
        return cls.for_shape(level, Tetromino.ALL[cell_value - 1])


class AssetForge:
    """Generates every texture and sound from code — no binary assets in the repo."""

    SAMPLE_RATE = 22050

    # Korobeiniki (Tetris Theme A), main loop: (note, beats). Rests are 'r'.
    NOTES = {'A4': 440.00, 'B4': 493.88, 'C5': 523.25, 'D5': 587.33, 'E5': 659.25,
             'F5': 698.46, 'G5': 783.99, 'A5': 880.00, 'r': 0.0}
    THEME = [
        ('E5', 1), ('B4', .5), ('C5', .5), ('D5', 1), ('C5', .5), ('B4', .5),
        ('A4', 1), ('A4', .5), ('C5', .5), ('E5', 1), ('D5', .5), ('C5', .5),
        ('B4', 1.5), ('C5', .5), ('D5', 1), ('E5', 1),
        ('C5', 1), ('A4', 1), ('A4', 1), ('r', 1),
        ('r', .5), ('D5', 1), ('F5', .5), ('A5', 1), ('G5', .5), ('F5', .5),
        ('E5', 1.5), ('C5', .5), ('E5', 1), ('D5', .5), ('C5', .5),
        ('B4', 1), ('B4', .5), ('C5', .5), ('D5', 1), ('E5', 1),
        ('C5', 1), ('A4', 1), ('A4', 1), ('r', 1),
    ]

    def __init__(self, directory):
        self.dir = directory
        os.makedirs(self.dir, exist_ok=True)

    def path(self, name):
        return os.path.join(self.dir, name)

    # ── textures ─────────────────────────────────────────────────────────────
    def build_textures(self):
        self._png('white.png', 1, 1, bytes([255, 255, 255, 255]))
        self._png('block.png', BLOCK_TEX, BLOCK_TEX, self._block_pixels(BLOCK_TEX))

    @staticmethod
    def _block_pixels(n):
        """A white block with a baked NES-style bevel; the tint multiplies it."""
        buf = bytearray()
        for y in range(n):
            for x in range(n):
                if x == n - 1 or y == n - 1:
                    v = 0.45          # bottom/right shadow
                elif x == 0 or y == 0:
                    v = 0.95          # top/left edge
                elif x <= 2 and y <= 2:
                    v = 1.00          # inner highlight square
                else:
                    v = 0.80          # body
                c = int(v * 255)
                buf.extend([c, c, c, 255])
        return bytes(buf)

    def _png(self, name, w, h, rgba):
        p = self.path(name)
        if os.path.exists(p):
            return
        def chunk(tag, data):
            return (struct.pack('>I', len(data)) + tag + data
                    + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))
        rows = b''.join(b'\x00' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
        with open(p, 'wb') as f:
            f.write(b'\x89PNG\r\n\x1a\n'
                    + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
                    + chunk(b'IDAT', zlib.compress(rows, 9))
                    + chunk(b'IEND', b''))

    # ── audio ────────────────────────────────────────────────────────────────
    def build_audio(self):
        sfx = {
            Event.MOVE:      self._square(220, 0.03, 0.4),
            Event.ROTATE:    self._square(330, 0.04, 0.4),
            Event.LOCK:      self._square(110, 0.05, 0.5),
            Event.LINE:      self._square(440, 0.08, 0.5) + self._square(660, 0.10, 0.5),
            Event.TETRIS:    (self._square(523, 0.05, 0.5) + self._square(659, 0.05, 0.5)
                              + self._square(784, 0.05, 0.5) + self._square(1046, 0.12, 0.5)),
            Event.LEVEL_UP:  self._square(659, 0.06, 0.5) + self._square(880, 0.12, 0.5),
            Event.GAME_OVER: (self._square(440, 0.12, 0.5) + self._square(311, 0.14, 0.5)
                              + self._square(220, 0.30, 0.5)),
        }
        for name, samples in sfx.items():
            self._wav(name + '.wav', samples)
        self._wav('music.wav', self._theme())

    def _theme(self, beat=0.26):
        samples = []
        for note, beats in self.THEME:
            freq = self.NOTES.get(note, 0.0)
            body = self._square(freq, max(0.02, beats * beat - 0.02), 0.28, decay=False)
            samples += body + self._square(0, 0.02)   # tiny gap so notes articulate
        return samples

    def _square(self, freq, duration, volume=0.5, decay=True):
        n = int(self.SAMPLE_RATE * duration)
        if freq <= 0:
            return [0] * n
        period = self.SAMPLE_RATE / freq
        out = []
        for i in range(n):
            envelope = (1.0 - i / n) if decay else 1.0
            s = volume * envelope * (1.0 if (i % period) < period / 2 else -1.0)
            out.append(int(max(-1.0, min(1.0, s)) * 32767))
        return out

    def _wav(self, name, samples):
        p = self.path(name)
        if os.path.exists(p):
            return
        with wave.open(p, 'w') as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(self.SAMPLE_RATE)
            w.writeframes(struct.pack('<%dh' % len(samples), *samples))


class BlockPool:
    """A reusable pool of bevelled block sprites, positioned on demand."""

    def __init__(self, scene, texture, size, count):
        self.size = size
        self.sprites = []
        scale = size / BLOCK_TEX
        for _ in range(count):
            s = loom.SpriteNode(texture)
            s.origin = loom.Vec2(0, 0)
            s.scale = loom.Vec2(scale, scale)
            s.visible = False
            scene.add(s)
            self.sprites.append(s)

    def __getitem__(self, i):
        return self.sprites[i]

    def place(self, i, x, y, color):
        s = self.sprites[i]
        # s.x / s.y rather than s.position = Vec2(...): this runs for every
        # visible cell every frame, and a Vec2 per sprite per frame is the one
        # allocation pattern loom2d's own batcher benchmarks warn about.
        s.x = x
        s.y = y
        s.tint = color
        s.visible = True

    def hide_from(self, i):
        for s in self.sprites[i:]:
            s.visible = False

    def hide_all(self):
        self.hide_from(0)


class WellView:
    """Draws the well: locked cells plus the live falling piece."""

    def __init__(self, scene, block_tex, x, y):
        self.x, self.y = x, y
        self.pool = BlockPool(scene, block_tex, CELL, Rules.COLS * Rules.ROWS)

    def render(self, engine):
        if engine is None:
            self.pool.hide_all()
            return

        level = engine.level
        active = {}
        if engine.piece is not None:
            for cx, cy in engine.piece.cells():
                if 0 <= cy < Rules.ROWS and 0 <= cx < Rules.COLS:
                    active[(cx, cy)] = engine.piece.shape

        i = 0
        for gy in range(Rules.ROWS):
            for gx in range(Rules.COLS):
                shape = active.get((gx, gy))
                if shape is not None:
                    color = Palette.for_shape(level, shape)
                elif engine.board.at(gx, gy):
                    color = Palette.for_cell(level, engine.board.at(gx, gy))
                else:
                    continue
                self.pool.place(i, self.x + gx * CELL, self.y + gy * CELL, color)
                i += 1
        self.pool.hide_from(i)


class NextBoxView:
    """The next-piece preview."""

    def __init__(self, scene, block_tex, x, y, w, h):
        self.x, self.y, self.w, self.h = x, y, w, h
        self.pool = BlockPool(scene, block_tex, MINI, 4)

    def render(self, engine):
        if engine is None:
            self.pool.hide_all()
            return
        shape = engine.next_shape
        color = Palette.for_shape(engine.level, shape)
        cx = self.x + self.w / 2 - MINI / 2
        cy = self.y + self.h / 2 - MINI / 2
        for i, (dx, dy) in enumerate(shape.offsets(0)):
            self.pool.place(i, cx + dx * MINI, cy + dy * MINI, color)


class StatsView:
    """The statistics panel: each tetromino's icon and how many have spawned."""

    ROW_HEIGHT = 54

    def __init__(self, game, scene, block_tex, font, x, y):
        self.x, self.y = x, y
        self.pool = BlockPool(scene, block_tex, MINI, 4 * len(Tetromino.ALL))
        self.labels = []
        for i in range(len(Tetromino.ALL)):
            ly = y + i * self.ROW_HEIGHT + MINI - 4
            self.labels.append(game.make_text(font, "000", x + 5 * MINI, ly,
                                              Palette.rgb((0xF8, 0x58, 0x58))))

    def render(self, engine):
        level = engine.level if engine else 0
        i = 0
        for shape in Tetromino.ALL:
            offsets = shape.offsets(0)
            color = Palette.for_shape(level, shape)
            top = min(dy for _, dy in offsets)
            oy = self.y + shape.index * self.ROW_HEIGHT - top * MINI
            ox = self.x + MINI
            for dx, dy in offsets:
                self.pool.place(i, ox + dx * MINI, oy + dy * MINI, color)
                i += 1
            count = engine.statistics[shape.index] if engine else 0
            self.labels[shape.index].text = f"{count:03d}"


class HudView:
    """LINES across the top, and TOP / SCORE / LEVEL down the right."""

    def __init__(self, game, fonts, next_box):
        white = loom.Color.white()
        gray = loom.Color(0.75, 0.75, 0.80, 1.0)
        big, med, small = fonts
        rx = 470

        self.lines = game.make_text(med, "LINES-000", BX, 60, white)
        game.make_text(small, "TOP", rx, 100, gray)
        self.top = game.make_text(med, "000000", rx, 120, white)
        game.make_text(small, "SCORE", rx, 168, gray)
        self.score = game.make_text(med, "000000", rx, 188, white)
        game.make_text(small, "NEXT", next_box.x, next_box.y - 26, gray)
        ly = next_box.y + next_box.h + 24
        game.make_text(small, "LEVEL", rx, ly, gray)
        self.level = game.make_text(med, "00", rx, ly + 20, white)
        game.make_text(small, "STATISTICS", 24, 118, gray)

    def render(self, engine, menu_level, top_score):
        score = engine.score if engine else 0
        lines = engine.lines if engine else 0
        level = engine.level if engine else menu_level
        self.lines.text = f"LINES-{lines:03d}"
        self.score.text = f"{score:06d}"
        self.top.text = f"{max(top_score, score):06d}"
        self.level.text = f"{level:02d}"


class Overlays:
    """Title / level-select, PAUSE and GAME OVER text, shown per screen."""

    def __init__(self, game, fonts):
        big, med, small = fonts
        white = loom.Color.white()
        self.title = game.make_text(big, "T E T R I S", BX - 6, 30,
                                    Palette.rgb((0xF8, 0xB8, 0x00)))
        self.level = game.make_text(big, "LEVEL 0", BX + 40, BY + 60,
                                    Palette.rgb((0x3C, 0xBC, 0xFC)))
        self.help = game.make_text(
            small,
            "Select level with Left/Right\n(Up/Down = +/-5)\n\nENTER to start",
            BX + 8, BY + 150, white)
        self.pause = game.make_text(big, "PAUSE", BX + 44, BY + 210, white)
        self.over = game.make_text(big, "GAME OVER", BX + 6, BY + 190,
                                   Palette.rgb((0xF8, 0x38, 0x00)))
        self.retry = game.make_text(small, "ENTER to play again", BX + 12, BY + 240, white)
        # Only shown when a pad is actually plugged in (it's hot-pluggable, so
        # this is re-checked every frame).
        self.pad = game.make_text(small, "GAMEPAD READY", BX + 8, BY + 260,
                                  Palette.rgb((0x58, 0xF8, 0x98)))
        self._menu = (self.title, self.level, self.help)

    def render(self, mode, menu_level, gamepad=False):
        in_menu = mode == TetrisGame.MENU
        for node in self._menu:
            node.visible = in_menu
        if in_menu:
            self.level.text = f"LEVEL {menu_level}"
        self.pad.visible = in_menu and gamepad
        self.pause.visible = mode == TetrisGame.PAUSED
        self.over.visible = mode == TetrisGame.OVER
        self.retry.visible = mode == TetrisGame.OVER


class TetrisFlash:
    """The NES screen flash on a TETRIS (a 4-line clear).

    On the original, clearing four rows at once strobes the whole background —
    it's the game's one moment of fanfare, and it's what makes a Tetris *feel*
    different from a triple. Nothing else in the game flashes.

    The strobe runs for exactly as long as the line-clear curtain (5 curtain
    steps x 4 frames = 20 frames), alternating every few frames. It is ticked on
    the fixed 60 Hz clock, like everything else, so it flashes at the same rate
    no matter the refresh rate.

    Only the *background* flips to white; the blocks keep their colours, so the
    stack stays readable through the strobe.
    """

    HALF_CYCLE = 3      # frames per half flash-cycle
    DURATION = Rules.CLEAR_STEP_FRAMES * LineClear.STEPS   # == the curtain, 20f
    BRIGHT = loom.Color(1.0, 1.0, 1.0, 1.0)

    def __init__(self, base_background, base_field):
        self.base_background = base_background
        self.base_field = base_field
        self.frames = 0
        self.active = False

    def trigger(self):
        self.active = True
        self.frames = 0

    def reset(self):
        self.active = False
        self.frames = 0

    def tick(self):
        """One fixed 60 Hz frame."""
        if not self.active:
            return
        self.frames += 1
        if self.frames >= self.DURATION:
            self.reset()

    @property
    def lit(self):
        return self.active and (self.frames // self.HALF_CYCLE) % 2 == 0

    def background(self):
        return self.BRIGHT if self.lit else self.base_background

    def field(self):
        return self.BRIGHT if self.lit else self.base_field


class SoundBank:
    """Turns engine Events into sounds; degrades silently with no audio device.

    Music and sound effects are muted independently (see SoundPanel).
    """

    def __init__(self, audio, forge):
        self.audio = audio
        self.forge = forge
        self.music_playing = False
        self.music_enabled = True
        self.sfx_enabled = True
        try:
            self.available = audio.initialized
        except Exception:
            self.available = False

    # ── sound effects ────────────────────────────────────────────────────────
    def play(self, event):
        if not (self.available and self.sfx_enabled):
            return
        try:
            self.audio.play_sound(self.forge.path(event + '.wav'), 0.6)
        except Exception:
            pass

    def handle(self, events):
        for e in events:
            self.play(e)

    def toggle_sfx(self):
        self.sfx_enabled = not self.sfx_enabled
        return self.sfx_enabled

    # ── music ────────────────────────────────────────────────────────────────
    def start_music(self):
        if not (self.available and self.music_enabled):
            return
        try:
            self.audio.play_music(self.forge.path('music.wav'), volume=0.5, loop=True)
            self.music_playing = True
        except Exception:
            pass

    def stop_music(self):
        if not (self.available and self.music_playing):
            return
        try:
            self.audio.stop_music()
        except Exception:
            pass
        self.music_playing = False

    def toggle_music(self, resume):
        """Flip music on/off. `resume` = restart the track now (i.e. mid-game)."""
        self.music_enabled = not self.music_enabled
        if self.music_enabled:
            if resume:
                self.start_music()
        else:
            self.stop_music()
        return self.music_enabled


class SoundPanel:
    """MUSIC / SFX on-off buttons, built with loom2d's screen-space UI toolkit.

    Two independent toggles: one for the Korobeiniki track, one for the sound
    effects. Also bound to the M and S keys.
    """

    ON_BG = (0x2C, 0x7A, 0x3C)
    OFF_BG = (0x7A, 0x2C, 0x2C)

    def __init__(self, game, font, sound):
        self.game = game
        self.sound = sound
        self.music_button = self._make(font, loom.Vec2(24, -58), self.toggle_music)
        self.sfx_button = self._make(font, loom.Vec2(24, -22), self.toggle_sfx)
        self.refresh()

    def _make(self, font, offset, on_click):
        button = loom.Button(font, "")
        button.anchor = loom.Vec2(0, 1)       # bottom-left of the screen
        button.pivot = loom.Vec2(0, 1)
        button.offset = offset
        button.size = loom.Vec2(150, 30)
        button.text_color = loom.Color.white()
        button.on_clicked = on_click
        button.enabled = self.sound.available
        self.game.ui.add(button)
        return button

    def toggle_music(self):
        self.sound.toggle_music(resume=self.game.mode == TetrisGame.PLAYING)
        self.refresh()

    def toggle_sfx(self):
        self.sound.toggle_sfx()
        self.refresh()

    def refresh(self):
        if not self.sound.available:
            self.music_button.caption = "NO AUDIO"
            self.sfx_button.caption = "NO AUDIO"
            return
        self._style(self.music_button, "MUSIC", self.sound.music_enabled)
        self._style(self.sfx_button, "SFX", self.sound.sfx_enabled)

    def _style(self, button, label, is_on):
        button.caption = f"{label}: {'ON' if is_on else 'OFF'}"
        base = self.ON_BG if is_on else self.OFF_BG
        button.bg = Palette.rgb(base)
        button.bg_hover = Palette.rgb(tuple(min(255, c + 40) for c in base))
        button.bg_pressed = Palette.rgb(tuple(max(0, c - 30) for c in base))


class HighScore:
    """The TOP score, persisted next to the example."""

    def __init__(self, path):
        self.path = path
        self.value = 0
        try:
            with open(path) as f:
                self.value = int(f.read().strip())
        except Exception:
            self.value = 0

    def submit(self, score):
        if score <= self.value:
            return False
        self.value = score
        try:
            with open(self.path, 'w') as f:
                f.write(str(int(score)))
        except Exception:
            pass
        return True


class Controls:
    """Player intent, read from the keyboard AND a gamepad — loom2d only.

    Everything comes from `loom.Input`: `key_*` for the keyboard, `gamepad_*` for
    the pad. The pad is mapped like the original NES controller:

        D-pad / left stick   move and soft-drop
        A (South) or B (East)    rotate clockwise      (the NES 'A' button)
        X (West) or Y (North)    rotate anticlockwise  (the NES 'B' button)
        Start                confirm / pause / restart

    Both rotate directions accept two face buttons, because "which button turns
    it right" is muscle memory that differs per player — this way either mental
    model works.

    Rotation is *latched*: a press is remembered until the fixed-timestep loop
    consumes it, so a press can never be lost on a frame that produces no
    simulation step (and never applied twice).
    """

    STICK = 0.5          # how far the stick must travel to count as a direction

    def __init__(self):
        loom.Input.set_gamepad_deadzone(0.25)
        self.pending_cw = False
        self.pending_ccw = False

    @property
    def gamepad_connected(self):
        return loom.Input.gamepad_connected(0)

    # ── per-render-frame edge latching ───────────────────────────────────────
    def poll(self):
        k = loom.Input
        if (k.key_pressed(loom.Key.X) or k.key_pressed(loom.Key.Up)
                or k.gamepad_pressed(loom.GamepadButton.South)
                or k.gamepad_pressed(loom.GamepadButton.East)):
            self.pending_cw = True
        if (k.key_pressed(loom.Key.Z) or k.key_pressed(loom.Key.Ctrl)
                or k.gamepad_pressed(loom.GamepadButton.West)
                or k.gamepad_pressed(loom.GamepadButton.North)):
            self.pending_ccw = True

    def take_rotation(self):
        """Consume the latched rotation edges (cw, ccw)."""
        cw, ccw = self.pending_cw, self.pending_ccw
        self.pending_cw = self.pending_ccw = False
        return cw, ccw

    def clear_rotation(self):
        self.pending_cw = self.pending_ccw = False

    # ── held directions (sampled every fixed step; DAS lives in the engine) ──
    @property
    def left(self):
        k = loom.Input
        return (k.key_down(loom.Key.Left)
                or k.gamepad_down(loom.GamepadButton.DpadLeft)
                or k.gamepad_axis(loom.GamepadAxis.LeftX) < -self.STICK)

    @property
    def right(self):
        k = loom.Input
        return (k.key_down(loom.Key.Right)
                or k.gamepad_down(loom.GamepadButton.DpadRight)
                or k.gamepad_axis(loom.GamepadAxis.LeftX) > self.STICK)

    @property
    def down(self):
        k = loom.Input
        return (k.key_down(loom.Key.Down)
                or k.gamepad_down(loom.GamepadButton.DpadDown)
                or k.gamepad_axis(loom.GamepadAxis.LeftY) > self.STICK)

    def inputs(self, cw=False, ccw=False):
        return Inputs(left=self.left, right=self.right, down=self.down,
                      cw=cw, ccw=ccw)

    # ── edges used by the menus ──────────────────────────────────────────────
    @property
    def confirm(self):
        # Start only — the face buttons rotate, so they must not also pause.
        return (loom.Input.key_pressed(loom.Key.Enter)
                or loom.Input.gamepad_pressed(loom.GamepadButton.Start))

    @property
    def menu_confirm(self):
        # Nothing rotates in a menu, so A may start the game there too.
        return self.confirm or loom.Input.gamepad_pressed(loom.GamepadButton.South)

    @property
    def menu_left(self):
        return (loom.Input.key_pressed(loom.Key.Left)
                or loom.Input.gamepad_pressed(loom.GamepadButton.DpadLeft))

    @property
    def menu_right(self):
        return (loom.Input.key_pressed(loom.Key.Right)
                or loom.Input.gamepad_pressed(loom.GamepadButton.DpadRight))

    @property
    def menu_up(self):
        return (loom.Input.key_pressed(loom.Key.Up)
                or loom.Input.gamepad_pressed(loom.GamepadButton.DpadUp))

    @property
    def menu_down(self):
        return (loom.Input.key_pressed(loom.Key.Down)
                or loom.Input.gamepad_pressed(loom.GamepadButton.DpadDown))

    # ── haptics ──────────────────────────────────────────────────────────────
    def rumble(self, low, high, ms):
        if self.gamepad_connected:
            loom.Input.gamepad_rumble(low, high, ms, 0)


class Autopilot:
    """Drives the game for --frames smoke tests so the play path renders."""

    def __init__(self):
        import random
        self.rng = random.Random(1234)

    def inputs(self):
        r = self.rng.random
        return Inputs(left=r() < 0.15, right=r() < 0.15, down=r() < 0.35,
                      cw=r() < 0.10, ccw=r() < 0.05)


class TetrisGame(loom.Game):
    """The loom2d game: owns the screens, the views, and the fixed 60 Hz clock."""

    MENU, PLAYING, PAUSED, OVER = 'menu', 'playing', 'paused', 'over'

    def on_start(self):
        self.logical_width = LOGICAL_W
        self.logical_height = LOGICAL_H
        self.scale_mode = loom.ScaleMode.Fit
        self.clear_color = BASE_BG
        self.scene.camera.position = loom.Vec2(LOGICAL_W / 2, LOGICAL_H / 2)

        self.forge = AssetForge(ASSETS)
        self.forge.build_textures()
        try:
            self.forge.build_audio()
        except Exception as exc:
            print("audio generation skipped:", exc)

        white_tex = self.assets.texture(self.forge.path('white.png'))
        block_tex = self.assets.texture(self.forge.path('block.png'))
        self.white_tex = white_tex

        fonts = self._load_fonts()

        # Draw order = insertion order, so: decor, then blocks, then text.
        self._build_decor()
        self.well = WellView(self.scene, block_tex, BX, BY)
        self.next_box = NextBoxView(self.scene, block_tex,
                                    self.nb_x, self.nb_y, self.nb_w, self.nb_h)
        self.stats = StatsView(self, self.scene, block_tex, fonts[2], 24, 150)
        self.hud = HudView(self, fonts, self.next_box)
        self.overlays = Overlays(self, fonts)

        self.sound = SoundBank(self.audio, self.forge)
        self.sound_panel = SoundPanel(self, fonts[2], self.sound)
        self.high_score = HighScore(os.path.join(HERE, 'hiscore.txt'))
        self.flash = TetrisFlash(BASE_BG, FIELD_BG)

        self.controls = Controls()
        self.engine = None
        self.mode = self.MENU
        self.menu_level = _arg_level()
        self.accum = 0.0

        self.frames_rendered = 0
        self.max_frames = _arg_frames()
        # --frames is a render smoke test: start at once and autopilot, so the
        # gameplay draw path (well, falling piece, next box, stats) is exercised.
        self.autopilot = Autopilot() if self.max_frames else None

        self._render()
        print("NES Tetris (loom2d) - ENTER to start. Arrows move, X/Z rotate, "
              "Down soft-drops.")
        if self.autopilot:
            self._start_game(self.menu_level)

    # ── scene construction helpers ───────────────────────────────────────────
    def make_text(self, font, text, x, y, color=None):
        node = loom.TextNode(font, text)
        node.origin = loom.Vec2(0, 0)
        node.position = loom.Vec2(x, y)
        node.color = color or loom.Color.white()
        self.scene.add(node)
        return node

    def _rect(self, x, y, w, h, color):
        s = loom.SpriteNode(self.white_tex)
        s.origin = loom.Vec2(0, 0)
        s.position = loom.Vec2(x, y)
        s.scale = loom.Vec2(w, h)
        s.tint = color
        self.scene.add(s)
        return s

    def _build_decor(self):
        border = Palette.rgb((0x9C, 0x9C, 0x9C))
        inner = loom.Color(0.02, 0.02, 0.03, 1.0)
        black = loom.Color(0, 0, 0, 1)

        self._rect(BX - 6, BY - 6, FIELD_W + 12, FIELD_H + 12, border)
        self._rect(BX - 3, BY - 3, FIELD_W + 6, FIELD_H + 6, inner)
        # kept: the Tetris flash strobes the well's backdrop white
        self.field_bg = self._rect(BX, BY, FIELD_W, FIELD_H, black)

        self.nb_x, self.nb_y = 470, 300
        self.nb_w, self.nb_h = 6 * MINI, 5 * MINI
        self._rect(self.nb_x - 6, self.nb_y - 6, self.nb_w + 12, self.nb_h + 12, border)
        self._rect(self.nb_x - 3, self.nb_y - 3, self.nb_w + 6, self.nb_h + 6, inner)
        self._rect(self.nb_x, self.nb_y, self.nb_w, self.nb_h, black)

    def _load_fonts(self):
        candidates = [
            "C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/cour.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "/System/Library/Fonts/Menlo.ttc",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        ]
        path = next((p for p in candidates if os.path.exists(p)), None)
        if not path:
            raise RuntimeError("no system font found")
        return (loom.Font.load(path, 26), loom.Font.load(path, 18), loom.Font.load(path, 14))

    # ── the loop ─────────────────────────────────────────────────────────────
    def on_update(self, dt):
        keys = loom.Input
        # Latch rotate presses at render rate so one is never lost on a frame
        # that produces no fixed step (high-refresh monitors).
        self.controls.poll()

        # Audio toggles: the on-screen buttons, or M / S.
        if keys.key_pressed(loom.Key.M):
            self.sound_panel.toggle_music()
        if keys.key_pressed(loom.Key.S):
            self.sound_panel.toggle_sfx()

        if self.mode == self.MENU:
            self._update_menu()
        elif self.mode == self.PAUSED:
            if self.controls.confirm:
                self.mode = self.PLAYING
        elif self.mode == self.OVER:
            if self.controls.confirm:
                self._enter_menu()
        elif self.mode == self.PLAYING:
            if self.controls.confirm and not self.autopilot:
                self._pause()
            else:
                self._advance(dt)

        self._render()

        self.frames_rendered += 1
        if self.max_frames and self.frames_rendered >= self.max_frames:
            score = self.engine.score if self.engine else 0
            lines = self.engine.lines if self.engine else 0
            print(f"frames={self.frames_rendered} mode={self.mode} score={score} "
                  f"lines={lines} draw_calls={self.last_draw_calls}")
            self.running = False

    def _update_menu(self):
        c = self.controls
        if c.menu_right:
            self.menu_level = min(9, self.menu_level + 1)
        if c.menu_left:
            self.menu_level = max(0, self.menu_level - 1)
        if c.menu_up:
            self.menu_level = min(9, self.menu_level + 5)
        if c.menu_down:
            self.menu_level = max(0, self.menu_level - 5)
        c.clear_rotation()          # Up / A also latched a rotate
        if c.menu_confirm:
            self._start_game(self.menu_level)

    def _advance(self, dt):
        """Step the engine on a fixed 60 Hz clock, independent of refresh rate."""
        self.accum += min(dt, 4 * FRAME)     # cap catch-up after a stall
        # The latched rotation is consumed by the FIRST step only, so one press
        # is exactly one rotation however many steps this frame runs.
        cw, ccw = self.controls.take_rotation()
        while self.accum >= FRAME:
            self.accum -= FRAME
            if self.autopilot:
                inputs = self.autopilot.inputs()
            else:
                inputs = self.controls.inputs(cw=cw, ccw=ccw)
            cw = ccw = False

            events = self.engine.step(inputs)
            self.sound.handle(events)
            if Event.TETRIS in events:
                self.flash.trigger()        # the NES screen flash on a 4-line clear
                self.controls.rumble(0.6, 0.9, 260)     # and a jolt in your hands
            self.flash.tick()               # strobes on the same fixed 60 Hz clock

            if self.engine.game_over:
                self._on_game_over()
                break

    # ── screen transitions ───────────────────────────────────────────────────
    def _start_game(self, level):
        self.engine = TetrisEngine(start_level=level)
        self.mode = self.PLAYING
        self.accum = 0.0
        self.controls.clear_rotation()
        self.flash.reset()
        self.sound.start_music()

    def _pause(self):
        self.mode = self.PAUSED
        # The flash only ticks inside _advance, which a pause skips. Without
        # this, pausing mid-strobe would freeze the screen white for the whole
        # pause.
        self.flash.reset()

    def _enter_menu(self):
        self.mode = self.MENU
        self.flash.reset()
        self.sound.stop_music()

    def _on_game_over(self):
        self.mode = self.OVER
        self.flash.reset()
        self.sound.stop_music()
        self.controls.rumble(0.9, 0.9, 500)
        self.high_score.submit(self.engine.score)

    # ── draw ─────────────────────────────────────────────────────────────────
    def _render(self):
        live = self.engine if self.mode in (self.PLAYING, self.PAUSED, self.OVER) else None
        self.clear_color = self.flash.background()
        self.field_bg.tint = self.flash.field()
        self.well.render(live)
        self.next_box.render(live)
        self.stats.render(live)
        self.hud.render(live, self.menu_level, self.high_score.value)
        self.overlays.render(self.mode, self.menu_level,
                             self.controls.gamepad_connected)

    def on_stop(self):
        if self.engine:
            print(f"Final: score={self.engine.score} lines={self.engine.lines} "
                  f"level={self.engine.level}")


def _arg_frames():
    if "--frames" in sys.argv:
        i = sys.argv.index("--frames")
        if i + 1 < len(sys.argv):
            return int(sys.argv[i + 1])
    return 0


def _arg_level():
    if "--level" in sys.argv:
        i = sys.argv.index("--level")
        if i + 1 < len(sys.argv):
            return max(0, min(9, int(sys.argv[i + 1])))
    return 0


if __name__ == "__main__":
    loom.run(TetrisGame(), title="NES Tetris - loom2d", width=LOGICAL_W, height=LOGICAL_H)
