#include <gtest/gtest.h>
#include "math/mat4.hpp"
#include "graphics/camera.hpp"
#include "graphics/sprite_batcher.hpp"
#include "graphics/scaling.hpp"
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

// ── compute_scaling (responsive resolution — Phase 2.7) ──────────────────────

TEST(Scaling, ExactMatchIsFullViewport) {
    // Drawable equals the logical resolution: full surface, no bars, logical cam.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 800, 600);
    EXPECT_EQ(r.vp_x, 0); EXPECT_EQ(r.vp_y, 0);
    EXPECT_EQ(r.vp_w, 800); EXPECT_EQ(r.vp_h, 600);
    EXPECT_FLOAT_EQ(r.cam_w, 800.f);
    EXPECT_FLOAT_EQ(r.cam_h, 600.f);
}

TEST(Scaling, FitPillarboxesWiderSurface) {
    // 800x600 (4:3) onto 1200x600 (2:1): scale=1, centred, vertical bars at sides.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 1200, 600);
    EXPECT_EQ(r.vp_w, 800);
    EXPECT_EQ(r.vp_h, 600);
    EXPECT_EQ(r.vp_x, 200); // (1200-800)/2 — pillarbox bars
    EXPECT_EQ(r.vp_y, 0);
    EXPECT_FLOAT_EQ(r.cam_w, 800.f); // logical viewport unchanged
}

TEST(Scaling, FitLetterboxesTallerSurface) {
    // 800x600 onto 800x900: scale=1, centred, horizontal bars top/bottom.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 800, 900);
    EXPECT_EQ(r.vp_w, 800);
    EXPECT_EQ(r.vp_h, 600);
    EXPECT_EQ(r.vp_x, 0);
    EXPECT_EQ(r.vp_y, 150); // (900-600)/2
}

TEST(Scaling, FitScalesUpPreservingAspect) {
    // 2x HiDPI: 800x600 onto 1600x1200 → fills exactly, scale 2, no bars.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 1600, 1200);
    EXPECT_EQ(r.vp_x, 0); EXPECT_EQ(r.vp_y, 0);
    EXPECT_EQ(r.vp_w, 1600); EXPECT_EQ(r.vp_h, 1200);
}

TEST(Scaling, StretchFillsWholeSurface) {
    auto r = compute_scaling(ScaleMode::Stretch, 800, 600, 1200, 600);
    EXPECT_EQ(r.vp_x, 0); EXPECT_EQ(r.vp_y, 0);
    EXPECT_EQ(r.vp_w, 1200); EXPECT_EQ(r.vp_h, 600);
    EXPECT_FLOAT_EQ(r.cam_w, 800.f); // logical projection stretched → distorts
    EXPECT_FLOAT_EQ(r.cam_h, 600.f);
}

TEST(Scaling, ExpandFillsAndRevealsMoreWorld) {
    // 800x600 onto 1200x600: scale=min(1.5,1)=1, viewport full, camera widens.
    auto r = compute_scaling(ScaleMode::Expand, 800, 600, 1200, 600);
    EXPECT_EQ(r.vp_w, 1200);
    EXPECT_EQ(r.vp_h, 600);
    EXPECT_FLOAT_EQ(r.cam_w, 1200.f); // 1200/1 — more world horizontally
    EXPECT_FLOAT_EQ(r.cam_h, 600.f);  // unchanged on the tight axis
}

TEST(Scaling, PixelPerfectUsesIntegerScale) {
    // 320x240 onto 700x540: float scale ~2.18 → integer 2 → 640x480 centred.
    auto r = compute_scaling(ScaleMode::PixelPerfect, 320, 240, 700, 540);
    EXPECT_EQ(r.vp_w, 640);
    EXPECT_EQ(r.vp_h, 480);
    EXPECT_EQ(r.vp_x, 30); // (700-640)/2
    EXPECT_EQ(r.vp_y, 30); // (540-480)/2
}

TEST(Scaling, PixelPerfectNeverScalesBelowOne) {
    // Surface smaller than logical: clamp to 1x rather than 0.
    auto r = compute_scaling(ScaleMode::PixelPerfect, 320, 240, 200, 150);
    EXPECT_EQ(r.vp_w, 320);
    EXPECT_EQ(r.vp_h, 240);
}

// ── window_point_to_logical (pointer remap) ──────────────────────────────────

TEST(Scaling, PointerIdentityWhenUnscaled) {
    // Logical == window, no HiDPI: pointer passes through unchanged.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 800, 600);
    float lx, ly;
    window_point_to_logical(r, 800, 600, 800, 600, 123.f, 456.f, lx, ly);
    EXPECT_FLOAT_EQ(lx, 123.f);
    EXPECT_FLOAT_EQ(ly, 456.f);
}

TEST(Scaling, PointerSubtractsLetterboxBars) {
    // 800x600 onto 1200x600: 200px pillarbox each side. A pointer at the left
    // edge of the content (x=200) maps to logical x=0; window centre → centre.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 1200, 600);
    float lx, ly;
    window_point_to_logical(r, 1200, 600, 1200, 600, 200.f, 0.f, lx, ly);
    EXPECT_FLOAT_EQ(lx, 0.f);
    EXPECT_FLOAT_EQ(ly, 0.f);
    window_point_to_logical(r, 1200, 600, 1200, 600, 600.f, 300.f, lx, ly);
    EXPECT_FLOAT_EQ(lx, 400.f); // (600-200)/800*800
    EXPECT_FLOAT_EQ(ly, 300.f);
}

TEST(Scaling, PointerAccountsForHiDpiRatio) {
    // 800x600 logical, 2x display: window 800x600 points, drawable 1600x1200 px.
    // Fit fills exactly (scale 2); pointer at window centre → logical centre.
    auto r = compute_scaling(ScaleMode::Fit, 800, 600, 1600, 1200);
    float lx, ly;
    window_point_to_logical(r, 1600, 1200, 800, 600, 400.f, 300.f, lx, ly);
    EXPECT_FLOAT_EQ(lx, 400.f);
    EXPECT_FLOAT_EQ(ly, 300.f);
}

TEST(Scaling, PointerExpandReturnsWidenedRange) {
    // Expand: camera widens to draw_w/scale; pointer at the right window edge
    // maps to the full widened logical width, not the original logical width.
    auto r = compute_scaling(ScaleMode::Expand, 800, 600, 1200, 600);
    float lx, ly;
    window_point_to_logical(r, 1200, 600, 1200, 600, 1200.f, 600.f, lx, ly);
    EXPECT_FLOAT_EQ(lx, 1200.f); // cam_w == 1200 in this Expand case
    EXPECT_FLOAT_EQ(ly, 600.f);
}
