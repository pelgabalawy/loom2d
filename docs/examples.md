# Examples

The repository ships a set of runnable examples in
[`examples/`](https://github.com/pelgabalawy/loom2d/tree/main/examples). Each is a
single self-contained Python file that generates its own art at runtime (no binary
assets), so you can run any of them straight after installing.

```bash
python examples/<name>/main.py
```

| Example | Shows off | Notes |
|---------|-----------|-------|
| **hello_world** | the minimal `Game` + a sprite | start here |
| **platformer** | physics + collision **events** in a playable game | A/D + Space; collect 5 coins |
| **physics_events** | contacts, sensors, raycasts | console-only, runs headless |
| **flappy** | component-style game architecture | full game in one file |
| **coin_quest** | tilemaps + grid collision + a live HUD | top-down maze game |
| **tilemap** | scrolling a large culled tilemap | 100×100 map |
| **text_demo** | font loading, alignment, wrapping | spinning/scaling text |
| **responsive** | [responsive scaling](guides/responsive-scaling.md) across window sizes | resize it; Q/W/E/R switch scale modes |
| **input_demo** | [gamepad, touch, mouse-wheel & text input](guides/input.md) | stick/WASD move, wheel zoom, type a caption |
| **stress_test** | the GPU sprite batcher | 10k sprites, reports FPS + draw calls |

## Highlights

### platformer — a playable physics game

A blue player drops onto ground and floating platforms, with five collectible coins.
It demonstrates the whole [physics events](guides/physics-events.md) system in a real
game: event-driven ground detection (you can only jump while actually standing on
something) and coins implemented as sensor bodies that vanish on pickup.

```bash
python examples/platformer/main.py
# A / D or arrows to move, Space to jump, Esc to quit
```

It also accepts `--frames N` to auto-run and exit — handy for smoke tests.

### physics_events — runs anywhere

A console-only tour of contacts, sensors, and raycasts. It needs no window or GPU, so
it's the fastest way to confirm the physics engine works on a fresh install:

```bash
python examples/physics_events/main.py
```

### stress_test — see the batcher work

Pushes thousands of sprites through the [GPU batcher](guides/sprites.md#performance-gpu-batching)
and prints the frame rate and draw-call count, showing how same-texture sprites
collapse into a single draw call.

## Self-terminating mode

Several examples accept `--frames N`: they run for `N` frames, print diagnostics, and
exit on their own. This is how the project smoke-tests builds without a human closing
the window, and it's a useful pattern to copy into your own games.

```bash
python examples/coin_quest/main.py --frames 600
```

## Learning path

1. **hello_world** → the bare minimum
2. **physics_events** → understand events with no rendering noise
3. **platformer** → those events inside a real game
4. **coin_quest** / **flappy** → fuller games to read and remix
