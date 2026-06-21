#include <gtest/gtest.h>
#include "math/mat4.hpp"
#include "graphics/camera.hpp"
#include "graphics/sprite_batcher.hpp"
#include <cmath>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

using namespace loom;

// ── Mat4 ────────────────────────────────────────────────────────────────────

TEST(Mat4, IdentityLeavesPointUnchanged) {
    auto p = Mat4::identity().transform(3.f, 4.f);
    EXPECT_FLOAT_EQ(p[0], 3.f);
    EXPECT_FLOAT_EQ(p[1], 4.f);
    EXPECT_FLOAT_EQ(p[3], 1.f);
}

TEST(Mat4, Translate) {
    auto p = Mat4::translate(10.f, -5.f).transform(1.f, 1.f);
    EXPECT_FLOAT_EQ(p[0], 11.f);
    EXPECT_FLOAT_EQ(p[1], -4.f);
}

TEST(Mat4, Scale) {
    auto p = Mat4::scale(2.f, 3.f).transform(4.f, 5.f);
    EXPECT_FLOAT_EQ(p[0], 8.f);
    EXPECT_FLOAT_EQ(p[1], 15.f);
}

TEST(Mat4, RotateZ90) {
    auto p = Mat4::rotate_z(static_cast<float>(M_PI) / 2.f).transform(1.f, 0.f);
    EXPECT_NEAR(p[0], 0.f, 1e-5f);
    EXPECT_NEAR(p[1], 1.f, 1e-5f);
}

TEST(Mat4, MultiplyAppliesRightThenLeft) {
    // translate-then-scale: scale(2) * translate(10,0) applied to (0,0) -> (20,0)
    Mat4 m = Mat4::scale(2.f, 2.f) * Mat4::translate(10.f, 0.f);
    auto p = m.transform(0.f, 0.f);
    EXPECT_FLOAT_EQ(p[0], 20.f);
    EXPECT_FLOAT_EQ(p[1], 0.f);
}

// ── Camera view-projection (must preserve the documented coordinate semantics) ─

TEST(CameraViewProjection, WorldOriginMapsToTopLeftClip) {
    Camera cam(800, 600);
    cam.set_position({400.f, 300.f}); // centred
    auto p = cam.view_projection().transform(0.f, 0.f);
    EXPECT_NEAR(p[0], -1.f, 1e-4f); // left
    EXPECT_NEAR(p[1],  1.f, 1e-4f); // top (GL clip y-up)
}

TEST(CameraViewProjection, ViewportCornerMapsToBottomRightClip) {
    Camera cam(800, 600);
    cam.set_position({400.f, 300.f});
    auto p = cam.view_projection().transform(800.f, 600.f);
    EXPECT_NEAR(p[0],  1.f, 1e-4f);
    EXPECT_NEAR(p[1], -1.f, 1e-4f);
}

TEST(CameraViewProjection, CameraCentreMapsToClipOrigin) {
    Camera cam(800, 600);
    cam.set_position({400.f, 300.f});
    auto p = cam.view_projection().transform(400.f, 300.f);
    EXPECT_NEAR(p[0], 0.f, 1e-4f);
    EXPECT_NEAR(p[1], 0.f, 1e-4f);
}

TEST(CameraViewProjection, ZoomScalesAroundCentre) {
    Camera cam(800, 600);
    cam.set_position({400.f, 300.f});
    cam.set_zoom(2.f);
    // A point 100px right of centre maps to 200px (in clip terms) at 2x zoom.
    auto p = cam.view_projection().transform(500.f, 300.f);
    EXPECT_NEAR(p[0], 200.f / 400.f, 1e-4f); // 0.5
}

// ── build_sprite_quad (pure sprite geometry) ────────────────────────────────

TEST(SpriteQuad, FullTextureTopLeftOrigin) {
    SpriteQuad q = build_sprite_quad({100.f, 100.f}, 0.f, {1.f, 1.f}, {0.f, 0.f},
                                     Rect{0, 0, 0, 0}, 32, 32, false, false);
    // TL, TR, BR, BL
    EXPECT_FLOAT_EQ(q.pos[0].x, 100.f); EXPECT_FLOAT_EQ(q.pos[0].y, 100.f);
    EXPECT_FLOAT_EQ(q.pos[2].x, 132.f); EXPECT_FLOAT_EQ(q.pos[2].y, 132.f);
    EXPECT_FLOAT_EQ(q.uv[0][0], 0.f);   EXPECT_FLOAT_EQ(q.uv[0][1], 0.f);
    EXPECT_FLOAT_EQ(q.uv[2][0], 1.f);   EXPECT_FLOAT_EQ(q.uv[2][1], 1.f);
}

TEST(SpriteQuad, CenteredOrigin) {
    SpriteQuad q = build_sprite_quad({0.f, 0.f}, 0.f, {1.f, 1.f}, {0.5f, 0.5f},
                                     Rect{0, 0, 0, 0}, 32, 32, false, false);
    EXPECT_FLOAT_EQ(q.pos[0].x, -16.f); EXPECT_FLOAT_EQ(q.pos[0].y, -16.f);
    EXPECT_FLOAT_EQ(q.pos[2].x,  16.f); EXPECT_FLOAT_EQ(q.pos[2].y,  16.f);
}

TEST(SpriteQuad, ScaleEnlargesWorldSize) {
    SpriteQuad q = build_sprite_quad({0.f, 0.f}, 0.f, {2.f, 3.f}, {0.f, 0.f},
                                     Rect{0, 0, 0, 0}, 10, 10, false, false);
    EXPECT_FLOAT_EQ(q.pos[2].x, 20.f); // 10 * 2
    EXPECT_FLOAT_EQ(q.pos[2].y, 30.f); // 10 * 3
}

TEST(SpriteQuad, SourceSubRectUVs) {
    // right half of a 32-wide texture
    SpriteQuad q = build_sprite_quad({0.f, 0.f}, 0.f, {1.f, 1.f}, {0.f, 0.f},
                                     Rect{16, 0, 16, 32}, 32, 32, false, false);
    EXPECT_FLOAT_EQ(q.uv[0][0], 0.5f);
    EXPECT_FLOAT_EQ(q.uv[1][0], 1.0f);
}

TEST(SpriteQuad, FlipXSwapsHorizontalUVs) {
    SpriteQuad q = build_sprite_quad({0.f, 0.f}, 0.f, {1.f, 1.f}, {0.f, 0.f},
                                     Rect{0, 0, 0, 0}, 32, 32, true, false);
    EXPECT_FLOAT_EQ(q.uv[0][0], 1.f); // TL now samples right edge
    EXPECT_FLOAT_EQ(q.uv[1][0], 0.f); // TR now samples left edge
}

TEST(SpriteQuad, Rotation90) {
    SpriteQuad q = build_sprite_quad({0.f, 0.f}, static_cast<float>(M_PI) / 2.f,
                                     {1.f, 1.f}, {0.f, 0.f},
                                     Rect{0, 0, 0, 0}, 32, 32, false, false);
    // TR corner (32,0) rotates 90deg clockwise (y-down) to (0,32)
    EXPECT_NEAR(q.pos[1].x, 0.f,  1e-3f);
    EXPECT_NEAR(q.pos[1].y, 32.f, 1e-3f);
}
