# NES Tetris — loom2d

A faithful clone of the NES (NTSC) *Tetris*, written in pure Python on loom2d.

```bash
python examples/tetris_nes/main.py              # play
python examples/tetris_nes/main.py --level 9    # start on level 9
python examples/tetris_nes/main.py --frames 300 # self-terminating smoke test
```

No binary assets: the block texture, the sound effects and the music are all
generated from code at first run into `assets/`.

## Controls

| Key | Action |
|-----|--------|
| ← / → | move (16-frame DAS charge, then 6-frame auto-repeat) |
| ↓ | soft drop (+1 point per row) |
| `X` / ↑ | rotate clockwise — the NES **A** button |
| `Z` / `Ctrl` | rotate anticlockwise — the NES **B** button |
| `Enter` | confirm level · pause · restart after a top-out |
| ← / → in the menu | pick the starting level (0–9); ↑ / ↓ jump by 5 |
| `M` / `S` | mute music / mute sound effects |
| `Escape` | quit |

The **MUSIC** and **SFX** buttons in the bottom-left corner do the same two things
with the mouse. They're real loom2d UI widgets on the screen-space canvas.

## Gamepad

Hot-pluggable — plug one in at any time and the menu shows **GAMEPAD READY**.
Mapped like the original NES pad:

| Pad | Action |
|-----|--------|
| D-pad / left stick | move and soft-drop (DAS works exactly as on the keyboard) |
| **A** or **B** | rotate clockwise — the NES **A** button |
| **X** or **Y** | rotate anticlockwise — the NES **B** button |
| **Start** | confirm level · pause · restart |

Two face buttons per rotation direction on purpose: "which button turns it right"
is muscle memory that differs per player, so either mental model works. Start is
deliberately the *only* pause button — the face buttons rotate, so they must not
also pause mid-game.

The pad **rumbles on a Tetris** and on a top-out.

## What makes it *NES* Tetris

These are the details that separate it from a generic Tetris:

- **Nintendo Rotation System, no wall kicks.** A rotation that would collide simply
  fails — there is no kick table to slide the piece free.
- **The NES gravity table.** Frames-per-row by level: 48, 43, 38, 33, 28, 23, 18, 13,
  8, 6, then 5/5/5, 4/4/4, 3/3/3, 2 through level 28, and 1 from 29 (the killscreen).
- **DAS.** A press shifts once immediately, then the piece waits 16 frames before
  auto-repeating every 6. Charge is kept against a wall, so releasing into an open
  column shifts instantly — the standard NES tapping/DAS technique works.
- **Soft drop only.** One row per 2 frames. There is **no hard drop and no hold**,
  because the NES has neither. A Down held through a spawn is ignored until released.
- **Push-down points.** Soft-dropping pays 1 point per row, banked when the piece
  locks — so yes, the score moves without clearing a line, exactly as on the NES.
  Gravity alone never scores. And the drop must be *unbroken*: **release Down and the
  counter resets to zero**, forfeiting the rows you already pushed. Only a soft drop
  held all the way into the lock pays out.
- **NES scoring.** 40 / 100 / 300 / 1200 for 1–4 lines, multiplied by (level + 1),
  scored at the level you were on *before* the clear. Capped at 999999.
- **The NES level curve.** The first level-up needs
  `min(start×10 + 10, max(100, start×10 − 50))` lines, and every 10 lines after.
- **The NES randomiser.** Roll 0–7; if it repeats the last piece or comes up 7,
  reroll once. That's what makes long droughts feel the way they do — and it still
  repeats slightly more often than a uniform bag would.
- **The line-clear curtain.** Cleared rows empty from the centre outwards, a column
  pair every 4 frames, before the stack collapses.
- **The Tetris flash.** Clearing four rows at once strobes the whole background white
  for the length of the curtain — the game's one moment of fanfare. A single, double
  or triple doesn't flash, which is exactly what makes a Tetris *feel* different.
- **ARE (entry delay).** 10 frames when a piece locks near the floor, up to 18 near
  the ceiling.
- **Per-level palettes.** Each level has two colours plus white ({I, O, T} white,
  {J, L} one colour, {S, Z} the other), and the **whole field recolours** on a
  level-up, exactly like the original.
- **Statistics** in the ROM's piece order (T, J, Z, O, S, L, I), a next box, a
  persistent TOP score, and top-out when a piece cannot spawn.

## How it's put together

`tetris_core.py` — the rules, as plain Python objects with **no loom2d import**:

| Class | Role |
|-------|------|
| `Tetromino` | immutable shape data; all seven in `Tetromino.ALL` |
| `Piece` | a live tetromino; `moved()` / `rotated()` return new pieces |
| `Board` | the 10×20 well: collision, locking, row collapse |
| `LineClear` | the centre-out clear animation |
| `Randomizer` | the NES piece selector |
| `ScoreKeeper` | score, lines, and the level curve |
| `Statistics` | per-piece spawn counts |
| `Rules` | the NES timing and scoring tables |
| `TetrisEngine` | the state machine; `step(Inputs)` advances **one NES frame** and returns the `Event`s it raised |

`main.py` — presentation only: `WellView`, `NextBoxView`, `StatsView`, `HudView`,
`Overlays`, `SoundPanel`, `SoundBank`, `AssetForge`, and the `TetrisGame(loom.Game)`
that owns them.

Two things worth copying:

- **The engine is stepped on a fixed 60 Hz accumulator**, not on `dt`. NES timings are
  counted in frames, so a 144 Hz monitor would otherwise run the game 2.4× too fast.
  Rotation presses are latched at render rate so one press is never lost, and never
  applied twice.
- **The rules have no engine dependency**, so the whole game is testable without a
  window — the same split `coin_quest` uses.
