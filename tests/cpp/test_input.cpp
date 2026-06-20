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
