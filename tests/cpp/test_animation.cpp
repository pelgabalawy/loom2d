#include <gtest/gtest.h>
#include "scene/animation.hpp"

using namespace loom;

TEST(Animation, DefaultEmpty) {
    Animation a("idle");
    EXPECT_EQ(a.frame_count(), 0);
    EXPECT_EQ(a.frame_index(), 0);
    EXPECT_FALSE(a.finished());
    EXPECT_THROW(a.current_frame(), std::runtime_error);
}

TEST(Animation, AddFrame) {
    Animation a("run");
    a.add_frame(Rect{0.f, 0.f, 32.f, 32.f}, 0.1f);
    EXPECT_EQ(a.frame_count(), 1);
}

TEST(Animation, SingleFrameNeverAdvances) {
    Animation a("idle");
    a.add_frame(Rect{0.f, 0.f, 32.f, 32.f}, 0.1f);
    a.update(0.05f);
    EXPECT_EQ(a.frame_index(), 0);
}

TEST(Animation, AdvancesToNextFrame) {
    Animation a("run");
    a.add_frame(Rect{0.f,   0.f, 32.f, 32.f}, 0.1f);
    a.add_frame(Rect{32.f,  0.f, 32.f, 32.f}, 0.1f);
    a.add_frame(Rect{64.f,  0.f, 32.f, 32.f}, 0.1f);

    a.update(0.15f); // past first frame (0.1s)
    EXPECT_EQ(a.frame_index(), 1);
}

TEST(Animation, LoopsAtEnd) {
    Animation a("run", /*loop=*/true);
    a.add_frame(Rect{}, 0.1f);
    a.add_frame(Rect{}, 0.1f);

    a.update(0.25f); // past both frames
    EXPECT_EQ(a.frame_index(), 0); // back to start
    EXPECT_FALSE(a.finished());
}

TEST(Animation, NoLoopStopsAtLastFrame) {
    Animation a("death", /*loop=*/false);
    a.add_frame(Rect{}, 0.1f);
    a.add_frame(Rect{}, 0.1f);
    a.add_frame(Rect{}, 0.1f);

    a.update(1.0f); // way past all frames
    EXPECT_EQ(a.frame_index(), 2); // stuck at last
    EXPECT_TRUE(a.finished());
}

TEST(Animation, Reset) {
    Animation a("run", /*loop=*/false);
    a.add_frame(Rect{}, 0.1f);
    a.add_frame(Rect{}, 0.1f);

    a.update(1.0f);
    EXPECT_TRUE(a.finished());

    a.reset();
    EXPECT_EQ(a.frame_index(), 0);
    EXPECT_FALSE(a.finished());
}

TEST(Animation, AddStrip) {
    Animation a("walk");
    // 4-column strip on a 128x32 sheet
    a.add_strip(128, 32, 32, 32, 4);
    EXPECT_EQ(a.frame_count(), 4);

    const auto& f0 = a.current_frame();
    EXPECT_FLOAT_EQ(f0.source.x, 0.f);
    EXPECT_FLOAT_EQ(f0.source.w, 32.f);
}

TEST(Animation, FrameDuration) {
    Animation a("slow");
    a.add_frame(Rect{}, 1.0f);
    a.add_frame(Rect{}, 1.0f);

    a.update(0.5f); // half way through frame 0
    EXPECT_EQ(a.frame_index(), 0);

    a.update(0.6f); // now past 1.0s total
    EXPECT_EQ(a.frame_index(), 1);
}

TEST(Animation, CurrentFrameSource) {
    Animation a("test");
    Rect r0{0.f,  0.f, 16.f, 16.f};
    Rect r1{16.f, 0.f, 16.f, 16.f};
    a.add_frame(r0, 0.1f);
    a.add_frame(r1, 0.1f);

    EXPECT_EQ(a.current_frame().source, r0);
    a.update(0.15f);
    EXPECT_EQ(a.current_frame().source, r1);
}
