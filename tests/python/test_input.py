"""Phase 2.8 input breadth: gamepad, touch, mouse wheel, text input.

All tests use the headless injection API so they run with no real devices.
"""
import loom2d as loom


# ── Enums exist ───────────────────────────────────────────────────────────────

def test_gamepad_enums_exist():
    assert loom.GamepadButton.South is not None
    assert loom.GamepadButton.DpadUp is not None
    assert loom.GamepadAxis.LeftX is not None
    assert loom.GamepadAxis.TriggerRight is not None


def test_new_key_values():
    assert loom.Key.Backspace is not None
    assert loom.Key.Delete is not None


# ── Mouse wheel ───────────────────────────────────────────────────────────────

def test_mouse_wheel_inject():
    loom.Input.inject_mouse_wheel(loom.Vec2(0, 4))
    assert loom.Input.mouse_wheel().y == 4


def test_mouse_wheel_cleared_by_new_frame():
    loom.Input.inject_mouse_wheel(loom.Vec2(1, 1))
    loom.Input.new_frame()
    assert loom.Input.mouse_wheel().x == 0
    assert loom.Input.mouse_wheel().y == 0


# ── Text input ────────────────────────────────────────────────────────────────

def test_text_input_inject():
    loom.Input.inject_text_input("hello")
    assert loom.Input.text_input() == "hello"


def test_text_input_cleared_by_new_frame():
    loom.Input.inject_text_input("xyz")
    loom.Input.new_frame()
    assert loom.Input.text_input() == ""


# ── Gamepad ───────────────────────────────────────────────────────────────────

def test_gamepad_not_connected_by_default():
    loom.Input.inject_gamepad_remove(0)
    assert not loom.Input.gamepad_connected(0)
    assert not loom.Input.gamepad_down(loom.GamepadButton.South, 0)
    assert loom.Input.gamepad_axis(loom.GamepadAxis.LeftX, 0) == 0.0


def test_gamepad_connect_and_button():
    loom.Input.inject_gamepad_remove(0)
    loom.Input.inject_gamepad_add(0)
    assert loom.Input.gamepad_connected(0)
    loom.Input.inject_gamepad_button(0, loom.GamepadButton.Start, True)
    assert loom.Input.gamepad_down(loom.GamepadButton.Start, 0)
    assert loom.Input.gamepad_pressed(loom.GamepadButton.Start, 0)


def test_gamepad_axis_deadzone():
    loom.Input.inject_gamepad_remove(0)
    loom.Input.inject_gamepad_add(0)
    loom.Input.set_gamepad_deadzone(0.15)
    loom.Input.inject_gamepad_axis(0, loom.GamepadAxis.LeftY, 0.1)
    assert loom.Input.gamepad_axis(loom.GamepadAxis.LeftY, 0) == 0.0
    loom.Input.inject_gamepad_axis(0, loom.GamepadAxis.LeftY, 1.0)
    assert abs(loom.Input.gamepad_axis(loom.GamepadAxis.LeftY, 0) - 1.0) < 1e-5


def test_gamepad_remove():
    loom.Input.inject_gamepad_add(0)
    loom.Input.inject_gamepad_remove(0)
    assert not loom.Input.gamepad_connected(0)


# ── Touch ─────────────────────────────────────────────────────────────────────

def _drain_touches():
    for t in loom.Input.touches():
        loom.Input.inject_touch_release(t.id)
    loom.Input.new_frame()


def test_touch_began_and_active():
    _drain_touches()
    loom.Input.inject_touch(1, loom.Vec2(120, 60))
    assert loom.Input.touch_count() == 1
    began = loom.Input.touches_began()
    assert len(began) == 1
    assert began[0].id == 1
    assert began[0].position.x == 120
    loom.Input.inject_touch_release(1)


def test_touch_ended():
    _drain_touches()
    loom.Input.inject_touch(2, loom.Vec2(0, 0))
    loom.Input.inject_touch_release(2)
    assert loom.Input.touch_count() == 0
    ended = loom.Input.touches_ended()
    assert len(ended) == 1
    assert ended[0].id == 2


def test_touch_persists_across_frames():
    _drain_touches()
    loom.Input.inject_touch(3, loom.Vec2(5, 5))
    loom.Input.new_frame()
    assert loom.Input.touches_began() == []
    assert loom.Input.touch_count() == 1
    loom.Input.inject_touch_release(3)
