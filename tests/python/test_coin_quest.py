"""
Integration test for the coin_quest example's game logic.

Exercises the Phase 2.5 tilemap the way a real game does — world generation,
wall collision, coin placement, pickup, and the win condition — all headlessly
(tilemap grid collision needs no GPU). The rendering path itself is covered by
running examples/coin_quest/main.py --frames N on a machine with a display.
"""
import os, sys, math
import pytest

pytest.importorskip("loom2d_native")

# Make the example's GPU-free logic module importable.
sys.path.insert(0, os.path.join(os.path.dirname(__file__),
                                '..', '..', 'examples', 'coin_quest'))
import world as W
import loom2d as loom


def test_map_is_built_with_two_layers_and_collision():
    m = W.build_map()
    assert m.width == W.MAP_W and m.height == W.MAP_H
    assert m.layer_by_name("ground") is not None
    assert m.layer_by_name("walls") is not None
    # Border is solid all the way around.
    assert m.is_solid(0, 0)
    assert m.is_solid(W.MAP_W - 1, W.MAP_H - 1)


def test_spawn_area_is_clear():
    m = W.build_map()
    sx, sy = W.SPAWN
    assert not m.is_solid(sx, sy)
    # Hero box at spawn does not overlap any wall.
    assert not m.rect_overlaps_solid(W.hero_box(W.spawn_position()))


def test_build_map_is_deterministic():
    a, b = W.build_map(seed=42), W.build_map(seed=42)
    assert all(a.is_solid(x, y) == b.is_solid(x, y)
               for x in range(a.width) for y in range(a.height))


def test_coins_are_placed_on_walkable_non_spawn_tiles():
    m = W.build_map()
    coins = W.place_coins(m, count=W.N_COINS)
    assert len(coins) == W.N_COINS
    seen = set()
    for c in coins:
        t = m.world_to_tile(c)
        tx, ty = int(t.x), int(t.y)
        assert not m.is_solid(tx, ty)        # never inside a wall
        assert (tx, ty) != W.SPAWN           # never on the spawn tile
        assert (tx, ty) not in seen          # all distinct
        seen.add((tx, ty))


def test_collision_blocks_movement_into_a_wall():
    m = W.build_map()
    # Push hard left from spawn — the border wall must stop x from going negative.
    pos = W.spawn_position()
    for _ in range(200):
        pos = W.try_move(m, pos, -10.0, 0.0)
    assert not m.rect_overlaps_solid(W.hero_box(pos))  # never ends up inside a wall
    assert pos.x > 0                                   # stopped by the border


def test_collect_picks_up_coin_within_radius():
    coins = [loom.Vec2(100, 100), loom.Vec2(400, 400)]
    remaining, collected = W.collect(loom.Vec2(100 + W.COIN_RADIUS - 2, 100), coins)
    assert collected == [0]
    assert len(remaining) == 1 and remaining[0] == loom.Vec2(400, 400)


def test_collect_ignores_coins_outside_radius():
    coins = [loom.Vec2(100, 100)]
    remaining, collected = W.collect(loom.Vec2(100 + W.COIN_RADIUS + 5, 100), coins)
    assert collected == []
    assert len(remaining) == 1


def test_full_autopilot_playthrough_collects_reachable_coins():
    """Simulate the auto-pilot loop headlessly: greedily walk toward the nearest
    coin each step, collecting along the way. Proves the move+collect loop makes
    progress without ever phasing through walls."""
    m = W.build_map(seed=3)
    coins = W.place_coins(m, count=W.N_COINS, seed=3)
    start = len(coins)
    pos = W.spawn_position()
    speed = 200.0
    dt = 1.0 / 60.0

    for _ in range(4000):
        if not coins:
            break
        target = min(coins, key=lambda c: (c.x - pos.x) ** 2 + (c.y - pos.y) ** 2)
        dx, dy = target.x - pos.x, target.y - pos.y
        mag = math.hypot(dx, dy)
        if mag > 0:
            step = speed * dt
            pos = W.try_move(m, pos, dx / mag * step, dy / mag * step)
        coins, collected = W.collect(pos, coins)
        # Invariant: the hero is never standing inside a solid tile.
        assert not m.rect_overlaps_solid(W.hero_box(pos))

    # Greedy pathing can't reach coins walled off behind stone, but on an open
    # seed it should collect a healthy majority — and definitely some.
    assert len(coins) < start
