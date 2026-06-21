"""
coin_quest — core game logic, deliberately GPU-free.

Everything here operates on loom2d primitives that work without a graphics
context (Tilemap grid collision, Vec2, Rect), so the whole play loop —
world generation, wall collision, coin pickup, win condition — can be unit
tested headlessly. main.py layers sprites/rendering on top of this.
"""
import os, sys, random

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'python'))
import loom2d as loom

TILE = 32
MAP_W, MAP_H = 40, 30
GRASS, WALL = 1, 2          # tile gids
HERO_SIZE = 20              # hero collision box (px)
COIN_RADIUS = 22            # pickup distance (px)
N_COINS = 12
SPAWN = (3, 3)              # spawn tile (kept clear of walls)


def build_map(seed=1):
    """A grass field walled off at the border with a random scattering of
    stone blocks. The spawn area is carved clear so the hero is never stuck."""
    m = loom.Tilemap(TILE, TILE, MAP_W, MAP_H)
    ground = m.add_layer("ground")
    walls = m.add_layer("walls")
    rng = random.Random(seed)
    for ty in range(MAP_H):
        for tx in range(MAP_W):
            ground.set(tx, ty, GRASS)
            border = tx == 0 or ty == 0 or tx == MAP_W - 1 or ty == MAP_H - 1
            if border or rng.random() < 0.12:
                walls.set(tx, ty, WALL)
    for ty in range(SPAWN[1] - 1, SPAWN[1] + 3):
        for tx in range(SPAWN[0] - 1, SPAWN[0] + 3):
            walls.set(tx, ty, 0)
    m.set_collision_from_layer(1)  # walls layer -> solid grid
    return m


def tile_center(tx, ty):
    return loom.Vec2(tx * TILE + TILE / 2, ty * TILE + TILE / 2)


def spawn_position():
    return tile_center(*SPAWN)


def walkable_cells(m):
    return [(tx, ty) for ty in range(m.height) for tx in range(m.width)
            if not m.is_solid(tx, ty)]


def place_coins(m, count=N_COINS, seed=2):
    """Pick `count` distinct walkable tiles (never the spawn) for coins.
    Returns a list of world-space center positions (Vec2)."""
    cells = [c for c in walkable_cells(m) if c != SPAWN]
    rng = random.Random(seed)
    rng.shuffle(cells)
    return [tile_center(tx, ty) for (tx, ty) in cells[:count]]


def hero_box(pos):
    h = HERO_SIZE / 2
    return loom.Rect(pos.x - h, pos.y - h, HERO_SIZE, HERO_SIZE)


def try_move(m, pos, dx, dy):
    """Axis-separated movement against the tilemap's solid grid, so the hero
    slides along walls instead of sticking. Returns the new Vec2 position."""
    x, y = pos.x, pos.y
    if not m.rect_overlaps_solid(hero_box(loom.Vec2(x + dx, y))):
        x += dx
    if not m.rect_overlaps_solid(hero_box(loom.Vec2(x, y + dy))):
        y += dy
    return loom.Vec2(x, y)


def collect(pos, coins):
    """Split `coins` into (still_remaining, indices_collected) by pickup radius."""
    remaining, collected = [], []
    r2 = COIN_RADIUS * COIN_RADIUS
    for i, c in enumerate(coins):
        if (c.x - pos.x) ** 2 + (c.y - pos.y) ** 2 < r2:
            collected.append(i)
        else:
            remaining.append(c)
    return remaining, collected
