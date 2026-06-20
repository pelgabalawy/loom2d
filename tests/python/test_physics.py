"""
Python-level tests for the physics bindings.
Box2D does not need a GPU or display — safe to run headlessly.
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import PhysicsWorld, BodyType, Vec2


class TestPhysicsWorld:
    def test_creation(self):
        w = PhysicsWorld()
        assert w is not None

    def test_custom_gravity(self):
        w = PhysicsWorld(gravity_x=0, gravity_y=500)
        assert w is not None


class TestPhysicsBody:
    def test_create_static(self):
        w = PhysicsWorld()
        b = w.create_body(BodyType.Static, Vec2(0, 0))
        assert b is not None

    def test_create_dynamic(self):
        w = PhysicsWorld()
        b = w.create_body(BodyType.Dynamic, Vec2(100, 200))
        assert b is not None
        pos = b.position
        assert pos.x == pytest.approx(100.0, abs=1.0)
        assert pos.y == pytest.approx(200.0, abs=1.0)

    def test_add_box(self):
        w = PhysicsWorld()
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.add_box(16, 16)

    def test_add_circle(self):
        w = PhysicsWorld()
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.add_circle(8)

    def test_gravity_moves_dynamic_body(self):
        w = PhysicsWorld(gravity_x=0, gravity_y=980)
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.add_box(10, 10)
        for _ in range(60):
            w.step(1 / 60)
        assert b.position.y > 0  # fallen downward

    def test_static_body_does_not_move(self):
        w = PhysicsWorld()
        b = w.create_body(BodyType.Static, Vec2(0, 300))
        b.add_box(400, 10)
        start_y = b.position.y
        for _ in range(60):
            w.step(1 / 60)
        assert b.position.y == pytest.approx(start_y, abs=0.1)

    def test_set_position(self):
        w = PhysicsWorld(gravity_y=0)
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.set_position(Vec2(200, 150))
        pos = b.position
        assert pos.x == pytest.approx(200.0, abs=1.0)
        assert pos.y == pytest.approx(150.0, abs=1.0)

    def test_linear_velocity(self):
        w = PhysicsWorld(gravity_y=0)
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.add_circle(8)
        b.set_linear_velocity(Vec2(64, 0))
        w.step(1 / 60)
        assert b.position.x > 0

    def test_apply_impulse(self):
        w = PhysicsWorld(gravity_y=0)
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        b.add_box(10, 10)
        b.apply_impulse(Vec2(100, 0))
        w.step(1 / 60)
        assert b.position.x > 0

    def test_body_type_enum_values(self):
        assert BodyType.Static    is not None
        assert BodyType.Dynamic   is not None
        assert BodyType.Kinematic is not None
        assert BodyType.Static != BodyType.Dynamic
