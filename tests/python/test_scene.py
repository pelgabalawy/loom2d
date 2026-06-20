"""
Test the Scene, Node, Animation API from Python.
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Node, Scene, Animation, Rect, Vec2


class TestNode:
    def test_default_position(self):
        n = Node()
        assert n.position == Vec2(0, 0)

    def test_set_position(self):
        n = Node()
        n.position = Vec2(100, 200)
        assert n.position == Vec2(100, 200)

    def test_xy_properties(self):
        n = Node()
        n.x = 42.0
        n.y = 99.0
        assert n.x == pytest.approx(42.0)
        assert n.y == pytest.approx(99.0)

    def test_rotation(self):
        n = Node()
        n.rotation = 1.5
        assert n.rotation == pytest.approx(1.5)

    def test_scale(self):
        n = Node()
        n.scale = Vec2(2, 3)
        assert n.scale == Vec2(2, 3)

    def test_name(self):
        n = Node("hero")
        assert n.name == "hero"

    def test_visible_default(self):
        n = Node()
        assert n.visible is True

    def test_add_child(self):
        parent = Node("parent")
        child  = Node("child")
        parent.add_child(child)
        assert len(parent.children()) == 1

    def test_world_position_with_parent(self):
        parent = Node()
        child  = Node()
        parent.position = Vec2(100, 0)
        child.position  = Vec2(50,  0)
        parent.add_child(child)
        wp = child.world_position()
        assert wp.x == pytest.approx(150.0, abs=0.01)

    def test_remove_from_parent(self):
        parent = Node()
        child  = Node()
        parent.add_child(child)
        child.remove_from_parent()
        assert len(parent.children()) == 0


class TestScene:
    def test_empty_scene(self):
        s = Scene()
        assert len(s.root().children()) == 0

    def test_add_node(self):
        s = Scene()
        s.add(Node("hero"))
        assert len(s.root().children()) == 1

    def test_clear(self):
        s = Scene()
        s.add(Node())
        s.add(Node())
        s.clear()
        assert len(s.root().children()) == 0

    def test_update_no_crash(self):
        s = Scene()
        s.add(Node())
        s.update(0.016)


class TestAnimation:
    def test_construction(self):
        a = Animation("run")
        assert a.name == "run"
        assert a.frame_count() == 0

    def test_add_frame(self):
        a = Animation("idle")
        a.add_frame(Rect(0, 0, 32, 32), 0.1)
        assert a.frame_count() == 1

    def test_loop_default_true(self):
        a = Animation("run")
        assert a.loop is True

    def test_no_loop(self):
        a = Animation("death", loop=False)
        assert a.loop is False

    def test_update_advances_frame(self):
        a = Animation("run")
        a.add_frame(Rect(0,  0, 32, 32), 0.1)
        a.add_frame(Rect(32, 0, 32, 32), 0.1)
        a.update(0.15)
        assert a.frame_index() == 1

    def test_reset(self):
        a = Animation("run", loop=False)
        a.add_frame(Rect(), 0.1)
        a.add_frame(Rect(), 0.1)
        a.update(1.0)
        assert a.finished()
        a.reset()
        assert not a.finished()
        assert a.frame_index() == 0

    def test_add_strip(self):
        a = Animation("walk")
        a.add_strip(128, 32, 32, 32, 4)
        assert a.frame_count() == 4
