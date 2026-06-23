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

    def test_tag_round_trips(self):
        w = PhysicsWorld(gravity_y=0)
        b = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        assert b.tag == ""
        b.tag = "player"
        assert b.tag == "player"


class TestContactEvents:
    def _make_collision_world(self):
        w = PhysicsWorld(gravity_x=0, gravity_y=0)
        a = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        a.add_box(10, 10)
        a.tag = "a"
        b = w.create_body(BodyType.Static, Vec2(15, 0))
        b.add_box(10, 10)
        b.tag = "b"
        a.set_linear_velocity(Vec2(64, 0))
        return w, a, b

    def test_contact_begin_event_list(self):
        w, a, b = self._make_collision_world()
        saw = False
        for _ in range(120):
            w.step(1 / 60)
            if w.contact_begins:
                saw = True
                pair = w.contact_begins[0]
                tags = {pair.body_a.tag, pair.body_b.tag}
                assert tags == {"a", "b"}
                break
        assert saw

    def test_contact_callback(self):
        w, a, b = self._make_collision_world()
        hits = []
        w.on_contact_begin = lambda x, y: hits.append((x.tag, y.tag))
        for _ in range(120):
            w.step(1 / 60)
            if hits:
                break
        assert hits
        assert set(hits[0]) == {"a", "b"}

    def test_no_contact_when_apart(self):
        w = PhysicsWorld(gravity_y=0)
        a = w.create_body(BodyType.Dynamic, Vec2(0, 0))
        a.add_box(10, 10)
        b = w.create_body(BodyType.Static, Vec2(500, 0))
        b.add_box(10, 10)
        for _ in range(30):
            w.step(1 / 60)
        assert list(w.contact_begins) == []


class TestSensorEvents:
    def test_sensor_overlap_without_block(self):
        w = PhysicsWorld(gravity_x=0, gravity_y=0)
        sensor = w.create_body(BodyType.Static, Vec2(0, 0))
        sensor.add_box(20, 20, is_sensor=True)
        sensor.tag = "zone"
        mover = w.create_body(BodyType.Dynamic, Vec2(-100, 0))
        mover.add_box(8, 8)
        mover.set_linear_velocity(Vec2(200, 0))

        saw = False
        for _ in range(120):
            w.step(1 / 60)
            if w.sensor_begins:
                saw = True
                pair = w.sensor_begins[0]
                assert pair.sensor.tag == "zone"
        assert saw
        # Sensor does not block: mover slides all the way through.
        assert mover.position.x > 50


class TestRaycast:
    def test_raycast_hits(self):
        w = PhysicsWorld(gravity_y=0)
        wall = w.create_body(BodyType.Static, Vec2(100, 0))
        wall.add_box(10, 50)
        wall.tag = "wall"
        w.step(1 / 60)
        hit = w.raycast(Vec2(0, 0), Vec2(300, 0))
        assert hit.hit
        assert bool(hit) is True
        assert hit.body.tag == "wall"
        assert hit.point.x == pytest.approx(90, abs=2)
        assert 0.0 <= hit.fraction <= 1.0

    def test_raycast_misses(self):
        w = PhysicsWorld(gravity_y=0)
        wall = w.create_body(BodyType.Static, Vec2(100, 500))
        wall.add_box(10, 10)
        w.step(1 / 60)
        hit = w.raycast(Vec2(0, 0), Vec2(300, 0))
        assert not hit.hit
        assert hit.body is None
