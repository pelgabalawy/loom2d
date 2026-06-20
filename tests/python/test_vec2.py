import pytest
import math
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Vec2


class TestVec2:
    def test_default_zero(self):
        v = Vec2()
        assert v.x == pytest.approx(0.0)
        assert v.y == pytest.approx(0.0)

    def test_construction(self):
        v = Vec2(3.0, 4.0)
        assert v.x == pytest.approx(3.0)
        assert v.y == pytest.approx(4.0)

    def test_addition(self):
        r = Vec2(1, 2) + Vec2(3, 4)
        assert r.x == pytest.approx(4.0)
        assert r.y == pytest.approx(6.0)

    def test_subtraction(self):
        r = Vec2(5, 3) - Vec2(2, 1)
        assert r.x == pytest.approx(3.0)
        assert r.y == pytest.approx(2.0)

    def test_scalar_multiply(self):
        r = Vec2(2, 3) * 2.0
        assert r.x == pytest.approx(4.0)
        assert r.y == pytest.approx(6.0)

    def test_scalar_rmultiply(self):
        r = 3.0 * Vec2(1, 2)
        assert r.x == pytest.approx(3.0)
        assert r.y == pytest.approx(6.0)

    def test_scalar_divide(self):
        r = Vec2(6, 4) / 2.0
        assert r.x == pytest.approx(3.0)
        assert r.y == pytest.approx(2.0)

    def test_negation(self):
        r = -Vec2(1, -2)
        assert r.x == pytest.approx(-1.0)
        assert r.y == pytest.approx(2.0)

    def test_length(self):
        assert Vec2(3, 4).length() == pytest.approx(5.0)

    def test_length_sq(self):
        assert Vec2(3, 4).length_sq() == pytest.approx(25.0)

    def test_normalized(self):
        n = Vec2(3, 4).normalized()
        assert n.length() == pytest.approx(1.0, abs=1e-6)

    def test_dot_perpendicular(self):
        assert Vec2(1, 0).dot(Vec2(0, 1)) == pytest.approx(0.0)

    def test_dot_parallel(self):
        assert Vec2(1, 0).dot(Vec2(1, 0)) == pytest.approx(1.0)

    def test_distance(self):
        assert Vec2(0, 0).distance(Vec2(3, 4)) == pytest.approx(5.0)

    def test_lerp_midpoint(self):
        m = Vec2(0, 0).lerp(Vec2(10, 10), 0.5)
        assert m.x == pytest.approx(5.0)
        assert m.y == pytest.approx(5.0)

    def test_static_helpers(self):
        assert Vec2.zero().x == pytest.approx(0.0)
        assert Vec2.one().x  == pytest.approx(1.0)
        assert Vec2.up().y   == pytest.approx(-1.0)
        assert Vec2.down().y == pytest.approx(1.0)

    def test_equality(self):
        assert Vec2(1, 2) == Vec2(1, 2)
        assert Vec2(1, 2) != Vec2(1, 3)

    def test_mutability(self):
        v = Vec2(1, 2)
        v.x = 99.0
        assert v.x == pytest.approx(99.0)

    def test_repr(self):
        r = repr(Vec2(1.0, 2.0))
        assert "Vec2" in r
