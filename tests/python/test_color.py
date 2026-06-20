import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Color


class TestColor:
    def test_default_construction(self):
        c = Color()
        assert c.r == pytest.approx(0.0)
        assert c.g == pytest.approx(0.0)
        assert c.b == pytest.approx(0.0)
        assert c.a == pytest.approx(1.0)

    def test_explicit_rgba(self):
        c = Color(0.5, 0.25, 0.75, 0.9)
        assert c.r == pytest.approx(0.5)
        assert c.g == pytest.approx(0.25)
        assert c.b == pytest.approx(0.75)
        assert c.a == pytest.approx(0.9)

    def test_mutability(self):
        c = Color()
        c.r = 1.0
        assert c.r == pytest.approx(1.0)

    def test_white(self):
        c = Color.white()
        assert c.r == pytest.approx(1.0)
        assert c.g == pytest.approx(1.0)
        assert c.b == pytest.approx(1.0)
        assert c.a == pytest.approx(1.0)

    def test_black(self):
        c = Color.black()
        assert c.r == pytest.approx(0.0)
        assert c.g == pytest.approx(0.0)
        assert c.b == pytest.approx(0.0)

    def test_red(self):
        c = Color.red()
        assert c.r == pytest.approx(1.0)
        assert c.g == pytest.approx(0.0)
        assert c.b == pytest.approx(0.0)

    def test_transparent(self):
        c = Color.transparent()
        assert c.a == pytest.approx(0.0)

    def test_cornflower(self):
        c = Color.cornflower()
        assert c.r == pytest.approx(0.39, abs=0.01)
        assert c.b == pytest.approx(0.93, abs=0.01)

    def test_repr_is_string(self):
        c = Color(1.0, 0.0, 0.0, 1.0)
        r = repr(c)
        assert isinstance(r, str)
        assert "Color" in r
