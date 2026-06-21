"""
Test the Tilemap / TileLayer / Tileset API from Python.
Pure data + collision behaviour (no GPU needed).
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Tilemap, TileLayer, Tileset, Rect, Vec2, Scene


class TestTileLayer:
    def test_dimensions(self):
        l = TileLayer("ground", 4, 3)
        assert l.width == 4
        assert l.height == 3

    def test_set_and_get(self):
        l = TileLayer("ground", 4, 3)
        l.set(2, 1, 7)
        assert l.at(2, 1) == 7
        assert l.at(0, 0) == 0

    def test_out_of_bounds_reads_zero(self):
        l = TileLayer("ground", 4, 3)
        assert l.at(-1, 0) == 0
        assert l.at(4, 0) == 0

    def test_fill(self):
        l = TileLayer("ground", 2, 2)
        l.fill(5)
        assert l.at(0, 0) == 5
        assert l.at(1, 1) == 5


class TestTilemap:
    def test_construction(self):
        m = Tilemap(32, 32, 10, 8)
        assert m.tile_w == 32
        assert m.width == 10
        assert m.height == 8

    def test_add_layer(self):
        m = Tilemap(32, 32, 4, 4)
        ground = m.add_layer("ground")
        assert ground.name == "ground"
        assert m.layer(0).name == "ground"
        assert m.layer_by_name("ground").name == "ground"
        assert len(m.layers()) == 1

    def test_world_to_tile(self):
        m = Tilemap(32, 32, 10, 10)
        t = m.world_to_tile(Vec2(70, 33))
        assert t.x == pytest.approx(2.0)
        assert t.y == pytest.approx(1.0)

    def test_tile_to_world_is_top_left(self):
        m = Tilemap(32, 32, 10, 10)
        w = m.tile_to_world(3, 2)
        assert w.x == pytest.approx(96.0)
        assert w.y == pytest.approx(64.0)

    def test_solid_grid(self):
        m = Tilemap(32, 32, 5, 5)
        assert not m.is_solid(2, 2)
        m.set_solid(2, 2, True)
        assert m.is_solid(2, 2)
        assert not m.is_solid(99, 99)

    def test_collision_from_layer(self):
        m = Tilemap(32, 32, 4, 4)
        walls = m.add_layer("walls")
        walls.set(1, 1, 5)
        m.set_collision_from_layer(0)
        assert m.is_solid(1, 1)
        assert not m.is_solid(0, 0)

    def test_rect_overlaps_solid(self):
        m = Tilemap(32, 32, 10, 10)
        m.set_solid(3, 3, True)
        assert m.rect_overlaps_solid(Rect(100, 100, 10, 10))
        assert not m.rect_overlaps_solid(Rect(0, 0, 10, 10))

    def test_is_a_node(self):
        # Tilemap is a Node, so it can be added to a scene and positioned.
        m = Tilemap(16, 16, 2, 2)
        m.position = Vec2(50, 50)
        assert m.position == Vec2(50, 50)
        s = Scene()
        s.add(m)
        assert len(s.root().children()) == 1
