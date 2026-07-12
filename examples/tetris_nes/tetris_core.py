"""
tetris_core — the object model and rules engine for the NES Tetris clone.

Pure Python, no engine dependency: `main.py` owns everything visual and drives
these objects one NES frame at a time. The split keeps the rules testable on
their own and keeps loom2d types out of the simulation.

Object model
    Tetromino    immutable shape data for one of the 7 pieces (all 7 in .ALL)
    Piece        a live tetromino: shape + rotation + anchor (immutable-ish;
                 moved()/rotated() return new Pieces)
    Randomizer   the NES pseudo-random piece selector
    Board        the 10x20 well: occupancy, collision, locking, row collapse
    LineClear    the centre-out "curtain" clear animation
    ScoreKeeper  score / lines / level and the NES level curve
    Statistics   per-piece spawn counts (the NES stats panel)
    Rules        the NES timing + scoring tables
    Inputs       one frame of player intent
    TetrisEngine the state machine that ties it together; emits Events for SFX

Faithful to NES (NTSC) Tetris:
  * Nintendo Rotation System with NO wall kicks — a blocked rotation just fails.
  * NES gravity table (frames-per-row by level) at 60 Hz.
  * DAS: 16-frame initial charge, 6-frame auto-repeat, wall charge preserved.
  * Soft drop = 1 row / 2 frames and pays push-down points. No hard drop.
  * Scoring 40/100/300/1200 x (level+1), capped at 999999.
  * The NES level curve and the NES randomiser (roll 0-7, reroll once).
  * Line-clear curtain, ARE entry delay (10-18f by lock height), top-out.
"""


# ── Events the engine emits (the view turns these into sounds) ───────────────
class Event:
    MOVE = 'move'
    ROTATE = 'rotate'
    LOCK = 'lock'
    LINE = 'line'
    TETRIS = 'tetris'
    LEVEL_UP = 'level'
    GAME_OVER = 'gameover'


# ── Engine states ────────────────────────────────────────────────────────────
class State:
    FALLING = 'falling'
    ARE = 'are'          # entry delay between pieces
    CLEARING = 'clearing'
    GAME_OVER = 'gameover'


class Rules:
    """The NES timing and scoring tables."""

    COLS = 10
    ROWS = 20
    SPAWN_X = 5
    SPAWN_Y = 1

    # Frames the piece waits before falling one row, indexed by level.
    GRAVITY = ([48, 43, 38, 33, 28, 23, 18, 13, 8, 6] +   # 0-9
               [5, 5, 5] +                                 # 10-12
               [4, 4, 4] +                                 # 13-15
               [3, 3, 3] +                                 # 16-18
               [2] * 10 +                                  # 19-28
               [1])                                        # 29+

    DAS_DELAY = 16          # frames to fully charge auto-shift
    DAS_PERIOD = 6          # frames between auto-shift repeats
    SOFT_DROP_FRAMES = 2    # a held Down falls one row every 2 frames
    CLEAR_STEP_FRAMES = 4   # frames per column-pair of the clear curtain
    ARE_MIN, ARE_MAX = 10, 18

    LINE_SCORES = {0: 0, 1: 40, 2: 100, 3: 300, 4: 1200}
    MAX_SCORE = 999999

    @classmethod
    def gravity(cls, level):
        return cls.GRAVITY[min(level, len(cls.GRAVITY) - 1)]

    @classmethod
    def first_level_up(cls, start_level):
        """Lines needed for the first level-up (NES formula)."""
        return min(start_level * 10 + 10, max(100, start_level * 10 - 50))

    @classmethod
    def entry_delay(cls, lowest_row):
        """ARE: 10 frames near the floor, up to 18 near the ceiling."""
        return max(cls.ARE_MIN, min(cls.ARE_MAX, 18 - 2 * (lowest_row // 4)))


class Tetromino:
    """Immutable shape data for one tetromino. The 7 live in Tetromino.ALL."""

    # Rotation states: 4 (dx, dy) offsets from the anchor. Clockwise advances the
    # index. +x right, +y DOWN. Pieces with 2 states toggle; O has 1.
    # colour_slot: 0 = level colour 1, 1 = level colour 2, 2 = white.
    # Order below is the NES ROM order (drives the randomiser + stats panel).
    _DEFS = [
        ('T', 2, [[(-1, 0), (0, 0), (1, 0), (0, 1)],    # points down (spawn)
                  [(0, -1), (0, 0), (0, 1), (-1, 0)],   # points left
                  [(-1, 0), (0, 0), (1, 0), (0, -1)],   # points up
                  [(0, -1), (0, 0), (0, 1), (1, 0)]]),  # points right
        ('J', 0, [[(-1, -1), (-1, 0), (0, 0), (1, 0)],
                  [(0, -1), (1, -1), (0, 0), (0, 1)],
                  [(-1, 0), (0, 0), (1, 0), (1, 1)],
                  [(0, -1), (0, 0), (-1, 1), (0, 1)]]),
        ('Z', 1, [[(-1, -1), (0, -1), (0, 0), (1, 0)],
                  [(1, -1), (0, 0), (1, 0), (0, 1)]]),
        ('O', 2, [[(-1, 0), (0, 0), (-1, 1), (0, 1)]]),
        ('S', 1, [[(0, -1), (1, -1), (-1, 0), (0, 0)],
                  [(-1, -1), (-1, 0), (0, 0), (0, 1)]]),
        ('L', 0, [[(1, -1), (-1, 0), (0, 0), (1, 0)],
                  [(0, -1), (0, 0), (0, 1), (1, 1)],
                  [(-1, 0), (0, 0), (1, 0), (-1, 1)],
                  [(-1, -1), (0, -1), (0, 0), (0, 1)]]),
        ('I', 2, [[(-2, 0), (-1, 0), (0, 0), (1, 0)],
                  [(0, -2), (0, -1), (0, 0), (0, 1)]]),
    ]

    ALL = []        # populated below

    def __init__(self, index, letter, color_slot, states):
        self.index = index
        self.letter = letter
        self.color_slot = color_slot
        self._states = [tuple(s) for s in states]

    @property
    def rotation_count(self):
        return len(self._states)

    def offsets(self, rotation=0):
        return self._states[rotation % len(self._states)]

    def __repr__(self):
        return f"<Tetromino {self.letter}>"


Tetromino.ALL = [Tetromino(i, letter, slot, states)
                 for i, (letter, slot, states) in enumerate(Tetromino._DEFS)]


class Piece:
    """A live tetromino on the board: shape + rotation + anchor cell."""

    def __init__(self, shape, x, y, rotation=0):
        self.shape = shape
        self.x = x
        self.y = y
        self.rotation = rotation % shape.rotation_count

    @property
    def index(self):
        return self.shape.index

    @property
    def color_slot(self):
        return self.shape.color_slot

    def cells(self):
        return [(self.x + dx, self.y + dy)
                for dx, dy in self.shape.offsets(self.rotation)]

    def moved(self, dx, dy):
        return Piece(self.shape, self.x + dx, self.y + dy, self.rotation)

    def rotated(self, clockwise):
        step = 1 if clockwise else -1
        return Piece(self.shape, self.x, self.y, self.rotation + step)

    @classmethod
    def spawn(cls, shape):
        return cls(shape, Rules.SPAWN_X, Rules.SPAWN_Y, 0)


class Randomizer:
    """The NES piece selector: roll 0-7; reroll once on a repeat or on the 8th."""

    def __init__(self, rng=None):
        import random
        self.rng = rng or random.Random()
        self.previous = self.rng.randint(0, 6)

    def roll(self):
        r = self.rng.randint(0, 7)
        if r == 7 or r == self.previous:
            r = self.rng.randint(0, 6)
        self.previous = r
        return Tetromino.ALL[r]


class Board:
    """The well. Cells hold 0 (empty) or a tetromino index + 1."""

    def __init__(self, cols=Rules.COLS, rows=Rules.ROWS):
        self.cols = cols
        self.rows = rows
        self.clear()

    def clear(self):
        self._grid = [[0] * self.cols for _ in range(self.rows)]

    def at(self, x, y):
        return self._grid[y][x]

    def rows_iter(self):
        return enumerate(self._grid)

    def fits(self, piece):
        """True if the piece occupies only empty, in-bounds cells.

        Cells above the ceiling (y < 0) are allowed — a piece may poke out.
        """
        for x, y in piece.cells():
            if x < 0 or x >= self.cols or y >= self.rows:
                return False
            if y >= 0 and self._grid[y][x] != 0:
                return False
        return True

    def lock(self, piece):
        """Stamp the piece in. Returns (topped_out, lowest_row_used)."""
        topped_out = False
        lowest = 0
        for x, y in piece.cells():
            if y < 0:
                topped_out = True          # locked partly above the ceiling
            else:
                self._grid[y][x] = piece.index + 1
                lowest = max(lowest, y)
        return topped_out, lowest

    def full_rows(self):
        return [y for y in range(self.rows) if all(self._grid[y])]

    def blank(self, y, x):
        if 0 <= x < self.cols:
            self._grid[y][x] = 0

    def collapse(self, rows):
        """Delete the given rows and drop everything above them down."""
        for y in sorted(rows):
            del self._grid[y]
            self._grid.insert(0, [0] * self.cols)

    def fill_all(self):
        """Test helper: fill every cell (forces a top-out on the next spawn)."""
        self._grid = [[1] * self.cols for _ in range(self.rows)]


class LineClear:
    """The NES clear animation: blank a column pair per tick, centre outwards."""

    STEPS = 5   # 10 columns / 2 per step

    def __init__(self, rows):
        self.rows = rows
        self.step_index = 0
        self.timer = 0

    @property
    def count(self):
        return len(self.rows)

    @property
    def lowest_row(self):
        return max(self.rows)

    def tick(self, board):
        """Advance one frame. Returns True once the curtain has finished."""
        self.timer += 1
        if self.timer < Rules.CLEAR_STEP_FRAMES:
            return False
        self.timer = 0
        left = 4 - self.step_index
        right = 5 + self.step_index
        for y in self.rows:
            board.blank(y, left)
            board.blank(y, right)
        self.step_index += 1
        return self.step_index >= self.STEPS


class ScoreKeeper:
    """Score, line count and the NES level curve."""

    def __init__(self, start_level=0):
        self.start_level = max(0, start_level)
        self.level = self.start_level
        self.score = 0
        self.lines = 0

    def add_points(self, points):
        self.score = min(Rules.MAX_SCORE, self.score + points)

    def add_lines(self, n):
        """Credit n cleared lines. Returns True if this triggered a level-up."""
        self.add_points(Rules.LINE_SCORES[n] * (self.level + 1))  # scored at the OLD level
        self.lines += n
        new_level = self._level_for(self.lines)
        leveled = new_level > self.level
        self.level = new_level
        return leveled

    def _level_for(self, lines):
        threshold = Rules.first_level_up(self.start_level)
        if lines < threshold:
            return self.start_level
        return self.start_level + 1 + (lines - threshold) // 10


class Statistics:
    """How many of each tetromino have spawned (the NES statistics panel)."""

    def __init__(self):
        self.counts = [0] * len(Tetromino.ALL)

    def record(self, shape):
        self.counts[shape.index] += 1

    def __getitem__(self, index):
        return self.counts[index]

    @property
    def total(self):
        return sum(self.counts)


class Inputs:
    """One frame of player intent. left/right/down are held; cw/ccw are edges."""

    __slots__ = ('left', 'right', 'down', 'cw', 'ccw')

    def __init__(self, left=False, right=False, down=False, cw=False, ccw=False):
        self.left = bool(left)
        self.right = bool(right)
        self.down = bool(down)
        self.cw = bool(cw)
        self.ccw = bool(ccw)

    @classmethod
    def none(cls):
        return cls()


class TetrisEngine:
    """One game of NES Tetris, advanced exactly one 60 Hz frame at a time."""

    def __init__(self, start_level=0, rng=None):
        self.board = Board()
        self.randomizer = Randomizer(rng)
        self.score_keeper = ScoreKeeper(start_level)
        self.statistics = Statistics()

        self.events = []
        self.state = State.ARE
        self.piece = None
        self.next_shape = self.randomizer.roll()
        self.clearing = None          # a LineClear while the curtain runs

        # timers
        self._drop_timer = 0
        self._das = 0
        self._das_dir = 0
        self._push_down = 0           # unbanked soft-drop points
        self._soft_locked = False     # a Down held through spawn is ignored
        self._are_timer = 0
        self._are_frames = 0

        self._spawn()

    # ── read-only view for the renderer ──────────────────────────────────────
    @property
    def level(self):
        return self.score_keeper.level

    @property
    def score(self):
        return self.score_keeper.score

    @property
    def lines(self):
        return self.score_keeper.lines

    @property
    def game_over(self):
        return self.state == State.GAME_OVER

    # ── the frame tick ───────────────────────────────────────────────────────
    def step(self, inputs):
        """Advance one NES frame. Returns the events raised this frame."""
        self.events = []
        if self.state == State.GAME_OVER:
            return self.events

        if self.state == State.CLEARING:
            self._tick_clear()
        elif self.state == State.ARE:
            self._tick_are(inputs)
        else:
            self._tick_falling(inputs)
        return self.events

    def _tick_falling(self, inp):
        if inp.cw or inp.ccw:
            if self.try_rotate(bool(inp.cw)):
                self.events.append(Event.ROTATE)

        self._tick_das(inp)

        # A Down held across a spawn must be released before it soft-drops again.
        if not inp.down:
            self._soft_locked = False
            # NES: push-down points only pay for an UNBROKEN soft drop. Let go of
            # Down and the counter resets — you bank nothing for the rows you
            # already pushed. Only a drop held all the way to the lock scores.
            self._push_down = 0
        soft = inp.down and not self._soft_locked

        interval = Rules.gravity(self.level)
        if soft:
            interval = min(interval, Rules.SOFT_DROP_FRAMES)

        self._drop_timer += 1
        if self._drop_timer >= interval:
            self._drop_timer = 0
            if self.try_move(0, 1):
                if soft:
                    self._push_down += 1
            else:
                self._lock()

    # ── player actions ───────────────────────────────────────────────────────
    def try_move(self, dx, dy):
        if self.state != State.FALLING:
            return False
        candidate = self.piece.moved(dx, dy)
        if self.board.fits(candidate):
            self.piece = candidate
            return True
        return False

    def try_rotate(self, clockwise):
        if self.state != State.FALLING:
            return False
        candidate = self.piece.rotated(clockwise)
        if self.board.fits(candidate):
            self.piece = candidate
            return True
        return False        # NES has no wall kicks: a blocked rotation just fails

    # ── DAS (delayed auto shift) ─────────────────────────────────────────────
    def _tick_das(self, inp):
        direction = 0
        if inp.left and not inp.right:
            direction = -1
        elif inp.right and not inp.left:
            direction = 1

        if direction == 0:
            self._das = 0
            self._das_dir = 0
            return

        if direction != self._das_dir:
            # fresh press: shift once immediately, then start charging
            self._das_dir = direction
            self._das = 0
            if self.try_move(direction, 0):
                self.events.append(Event.MOVE)
            return

        self._das += 1
        if self._das >= Rules.DAS_DELAY:
            self._das = Rules.DAS_DELAY - Rules.DAS_PERIOD   # repeat every 6f
            if self.try_move(direction, 0):
                self.events.append(Event.MOVE)
            else:
                self._das = Rules.DAS_DELAY   # wall charge: retry every frame

    # ── locking, clearing, spawning ──────────────────────────────────────────
    def _lock(self):
        topped_out, lowest = self.board.lock(self.piece)
        self.events.append(Event.LOCK)

        # soft-drop push-down points bank at lock time
        self.score_keeper.add_points(self._push_down)
        self._push_down = 0

        if topped_out:
            self._end_game()
            return

        full = self.board.full_rows()
        if full:
            self.piece = None
            self.clearing = LineClear(full)
            self.state = State.CLEARING
            self.events.append(Event.TETRIS if len(full) == 4 else Event.LINE)
        else:
            self._begin_are(lowest)

    def _tick_clear(self):
        if not self.clearing.tick(self.board):
            return
        clear = self.clearing
        self.board.collapse(clear.rows)
        if self.score_keeper.add_lines(clear.count):
            self.events.append(Event.LEVEL_UP)
        self.clearing = None
        self._begin_are(clear.lowest_row)

    def _begin_are(self, lowest_row):
        self.state = State.ARE
        self._are_timer = 0
        self._are_frames = Rules.entry_delay(lowest_row)

    def _tick_are(self, inp):
        self._are_timer += 1
        if self._are_timer >= self._are_frames:
            self._spawn(inp.down)

    def _spawn(self, down_held=False):
        shape = self.next_shape
        self.next_shape = self.randomizer.roll()
        self.piece = Piece.spawn(shape)
        self.statistics.record(shape)

        self._drop_timer = 0
        self._push_down = 0
        self._soft_locked = bool(down_held)   # don't carry a held Down into it

        if not self.board.fits(self.piece):
            self._end_game()
        else:
            self.state = State.FALLING

    def _end_game(self):
        self.state = State.GAME_OVER
        self.piece = None
        self.events.append(Event.GAME_OVER)
