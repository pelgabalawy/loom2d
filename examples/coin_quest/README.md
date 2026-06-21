# coin_quest

A tiny top-down maze game, and the **end-to-end test for Phase 2.5 (tilemaps)**.
Navigate a 40×30 tile maze and collect all 12 coins.

It exercises, all at once:

- **Tilemap rendering** — a `ground` + `walls` layer drawn through the sprite batcher
- **Viewport culling** — only the tiles around the camera are submitted, so the
  draw cost barely changes as you roam (watch `tiles_drawn` in `--frames` mode)
- **Grid collision** — the hero can't walk through stone (axis-separated sliding)
- **Camera follow** + **sprite batching** — hero and coins share the draw path

## Run it

```bash
# from the repo root, after building (./build.ps1 / ./build.sh)
python examples/coin_quest/main.py
```

- **Move:** arrow keys or **WASD**
- **Goal:** collect all the coins — you win when the last one is picked up
- **Quit:** Esc

### Non-interactive smoke / render test

```bash
python examples/coin_quest/main.py --frames 600
```

Auto-pilots the hero toward the nearest coin, prints `score`, `tiles_drawn`, and
`draw_calls` once a second, then exits. Handy for confirming the GPU draw path
works on a new platform (macOS/Linux) without sitting at the keyboard.

> The actual rendering needs a real GL context, so it can't run under SDL's
> headless `offscreen` driver. That's why this runnable game is the manual
> verification step for Phase 2.5.

## How it's structured (and tested)

- **`world.py`** — all game *logic* (map generation, wall collision, coin
  placement, pickup), written against GPU-free loom2d primitives. Because
  `Tilemap` grid collision needs no graphics context, this is fully unit-tested
  headlessly in [`tests/python/test_coin_quest.py`](../../tests/python/test_coin_quest.py)
  — including a complete simulated playthrough.
- **`main.py`** — loads textures, builds sprites, draws, and handles input on top
  of `world.py`. This is the part that needs a display.
