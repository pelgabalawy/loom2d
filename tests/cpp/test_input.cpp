#include <gtest/gtest.h>
#include "input/input.hpp"

using namespace loom;

// Reset input state between tests via injection API
class InputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset all keys to up via a known key
        Input::inject_key_up(Key::Space);
        Input::inject_key_up(Key::W);
        Input::inject_key_up(Key::A);
    }
};

TEST_F(InputTest, KeyDefaultIsUp) {
    EXPECT_FALSE(Input::key_down(Key::Space));
}

TEST_F(InputTest, InjectKeyDown) {
    Input::inject_key_down(Key::Space);
    EXPECT_TRUE(Input::key_down(Key::Space));
}

TEST_F(InputTest, KeyPressedTrueOnFirstFrame) {
    // inject_key_down sets previous = current (before setting current = true)
    // so key_pressed detects the edge: previous=false, current=true
    Input::inject_key_up(Key::W);   // ensure it was up
    Input::inject_key_down(Key::W); // now press it
    EXPECT_TRUE(Input::key_pressed(Key::W));
    EXPECT_TRUE(Input::key_down(Key::W));
}

TEST_F(InputTest, KeyReleasedDetectsEdge) {
    Input::inject_key_down(Key::A); // press
    Input::inject_key_up(Key::A);   // release
    EXPECT_TRUE(Input::key_released(Key::A));
    EXPECT_FALSE(Input::key_down(Key::A));
}

TEST_F(InputTest, KeyPressedFalseWhenAlreadyHeld) {
    Input::inject_key_down(Key::Space); // first press
    // Second call to inject_key_down: prev = current = true → not a new press
    Input::inject_key_down(Key::Space);
    // Now previous=true, current=true → key_pressed = false
    EXPECT_FALSE(Input::key_pressed(Key::Space));
    EXPECT_TRUE(Input::key_down(Key::Space));
}

TEST_F(InputTest, MousePositionInjection) {
    Input::inject_mouse_position(Vec2{320.f, 240.f});
    Vec2 pos = Input::mouse_position();
    EXPECT_FLOAT_EQ(pos.x, 320.f);
    EXPECT_FLOAT_EQ(pos.y, 240.f);
}

TEST_F(InputTest, MultipleKeys) {
    Input::inject_key_down(Key::W);
    Input::inject_key_down(Key::D);
    EXPECT_TRUE(Input::key_down(Key::W));
    EXPECT_TRUE(Input::key_down(Key::D));
    EXPECT_FALSE(Input::key_down(Key::S));
}

// ── Mouse wheel ──────────────────────────────────────────────────────────────

TEST(InputWheel, InjectAccumulatedDelta) {
    Input::inject_mouse_wheel(Vec2{1.f, -3.f});
    EXPECT_FLOAT_EQ(Input::mouse_wheel().x, 1.f);
    EXPECT_FLOAT_EQ(Input::mouse_wheel().y, -3.f);
}

TEST(InputWheel, NewFrameClearsDelta) {
    Input::inject_mouse_wheel(Vec2{2.f, 2.f});
    Input::new_frame();
    EXPECT_FLOAT_EQ(Input::mouse_wheel().x, 0.f);
    EXPECT_FLOAT_EQ(Input::mouse_wheel().y, 0.f);
}

// ── Text input ───────────────────────────────────────────────────────────────

TEST(InputText, InjectTypedText) {
    Input::inject_text_input("hi");
    EXPECT_EQ(Input::text_input(), "hi");
}

TEST(InputText, NewFrameClearsText) {
    Input::inject_text_input("abc");
    Input::new_frame();
    EXPECT_EQ(Input::text_input(), "");
}

// ── Gamepad ──────────────────────────────────────────────────────────────────

class GamepadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start from a clean pad slot 0.
        Input::inject_gamepad_remove(0);
        Input::inject_gamepad_remove(1);
        Input::set_gamepad_deadzone(0.15f);
    }
};

TEST_F(GamepadTest, NotConnectedByDefault) {
    EXPECT_FALSE(Input::gamepad_connected(0));
    EXPECT_FALSE(Input::gamepad_down(GamepadButton::South, 0));
    EXPECT_FLOAT_EQ(Input::gamepad_axis(GamepadAxis::LeftX, 0), 0.f);
}

TEST_F(GamepadTest, AddMakesConnected) {
    Input::inject_gamepad_add(0);
    EXPECT_TRUE(Input::gamepad_connected(0));
    EXPECT_GE(Input::gamepad_count(), 1);
}

TEST_F(GamepadTest, ButtonDownAndPressedEdge) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_button(0, GamepadButton::South, true);
    EXPECT_TRUE(Input::gamepad_down(GamepadButton::South, 0));
    EXPECT_TRUE(Input::gamepad_pressed(GamepadButton::South, 0));
    // Holding it a second frame is no longer a fresh press.
    Input::inject_gamepad_button(0, GamepadButton::South, true);
    EXPECT_FALSE(Input::gamepad_pressed(GamepadButton::South, 0));
    EXPECT_TRUE(Input::gamepad_down(GamepadButton::South, 0));
}

TEST_F(GamepadTest, ButtonReleasedEdge) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_button(0, GamepadButton::East, true);
    Input::inject_gamepad_button(0, GamepadButton::East, false);
    EXPECT_TRUE(Input::gamepad_released(GamepadButton::East, 0));
    EXPECT_FALSE(Input::gamepad_down(GamepadButton::East, 0));
}

TEST_F(GamepadTest, AxisDeadzoneZeroesSmallValues) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_axis(0, GamepadAxis::LeftX, 0.1f); // below 0.15
    EXPECT_FLOAT_EQ(Input::gamepad_axis(GamepadAxis::LeftX, 0), 0.f);
}

TEST_F(GamepadTest, AxisDeadzoneRescalesAboveThreshold) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_axis(0, GamepadAxis::LeftX, 1.f);
    EXPECT_FLOAT_EQ(Input::gamepad_axis(GamepadAxis::LeftX, 0), 1.f); // full stays full
    Input::inject_gamepad_axis(0, GamepadAxis::LeftX, -1.f);
    EXPECT_FLOAT_EQ(Input::gamepad_axis(GamepadAxis::LeftX, 0), -1.f); // sign preserved
}

TEST_F(GamepadTest, TriggerNotDeadzoned) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_axis(0, GamepadAxis::TriggerLeft, 0.05f);
    EXPECT_FLOAT_EQ(Input::gamepad_axis(GamepadAxis::TriggerLeft, 0), 0.05f);
}

TEST_F(GamepadTest, RemoveDisconnects) {
    Input::inject_gamepad_add(0);
    Input::inject_gamepad_remove(0);
    EXPECT_FALSE(Input::gamepad_connected(0));
}

// ── Touch ────────────────────────────────────────────────────────────────────

class TouchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Drain any fingers left active by a prior test.
        for (auto& t : Input::touches()) Input::inject_touch_release(t.id);
        Input::new_frame();
    }
};

TEST_F(TouchTest, BeganAndActive) {
    Input::inject_touch(1, Vec2{100.f, 50.f});
    EXPECT_EQ(Input::touch_count(), 1);
    auto began = Input::touches_began();
    ASSERT_EQ(began.size(), 1u);
    EXPECT_EQ(began[0].id, 1);
    EXPECT_FLOAT_EQ(began[0].position.x, 100.f);
}

TEST_F(TouchTest, EndedRemovesFinger) {
    Input::inject_touch(2, Vec2{10.f, 10.f});
    Input::inject_touch_release(2);
    EXPECT_EQ(Input::touch_count(), 0);
    auto ended = Input::touches_ended();
    ASSERT_EQ(ended.size(), 1u);
    EXPECT_EQ(ended[0].id, 2);
}

TEST_F(TouchTest, NewFrameClearsBeganEnded) {
    Input::inject_touch(3, Vec2{5.f, 5.f});
    Input::new_frame();
    EXPECT_TRUE(Input::touches_began().empty());
    EXPECT_EQ(Input::touch_count(), 1); // still active across frames
    Input::inject_touch_release(3);
}
