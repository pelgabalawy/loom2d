#pragma once
#include <algorithm>
#include <cmath>

namespace loom {

// How a fixed logical (design) resolution is mapped onto the real drawable
// surface when the window/screen has a different size or aspect ratio.
enum class ScaleMode {
    Fit,          // letterbox/pillarbox: preserve aspect, bars fill the remainder (default)
    Stretch,      // fill the whole surface, distorting aspect if it differs
    Expand,       // preserve aspect, fill the surface by revealing more world
    PixelPerfect  // integer-scaled Fit (crisp pixel art), bars fill the remainder
};

// Result of resolving a scale mode for a given logical + drawable size.
//   vp_*  — the GPU viewport rectangle (device pixels, top-left origin) the game
//           should render into. For Fit/PixelPerfect this is centred and smaller
//           than the surface (the surround is letterbox bars).
//   cam_* — the logical viewport size to feed the camera projection. Equals the
//           logical resolution except for Expand, where it grows so more world
//           becomes visible instead of adding bars.
struct ScaleResult {
    int   vp_x = 0, vp_y = 0, vp_w = 0, vp_h = 0;
    float cam_w = 0.f, cam_h = 0.f;
};

// Pure, headless-testable: compute how a logical_w×logical_h design resolution
// maps onto a draw_w×draw_h drawable surface under the given scale mode.
inline ScaleResult compute_scaling(ScaleMode mode, int logical_w, int logical_h,
                                   int draw_w, int draw_h) {
    ScaleResult r;
    if (logical_w <= 0) logical_w = draw_w;
    if (logical_h <= 0) logical_h = draw_h;
    if (draw_w    <= 0) draw_w    = logical_w;
    if (draw_h    <= 0) draw_h    = logical_h;

    const float lw = static_cast<float>(logical_w);
    const float lh = static_cast<float>(logical_h);
    const float dw = static_cast<float>(draw_w);
    const float dh = static_cast<float>(draw_h);

    switch (mode) {
    case ScaleMode::Stretch:
        r.vp_x = 0; r.vp_y = 0; r.vp_w = draw_w; r.vp_h = draw_h;
        r.cam_w = lw; r.cam_h = lh;
        break;

    case ScaleMode::Expand: {
        // Same per-unit scale as Fit, but the viewport covers the whole surface
        // and the camera sees a correspondingly larger slice of the world.
        const float scale = std::min(dw / lw, dh / lh);
        r.vp_x = 0; r.vp_y = 0; r.vp_w = draw_w; r.vp_h = draw_h;
        r.cam_w = dw / scale;
        r.cam_h = dh / scale;
        break;
    }

    case ScaleMode::PixelPerfect: {
        int iscale = static_cast<int>(std::floor(std::min(dw / lw, dh / lh)));
        if (iscale < 1) iscale = 1;
        r.vp_w = logical_w * iscale;
        r.vp_h = logical_h * iscale;
        r.vp_x = (draw_w - r.vp_w) / 2;
        r.vp_y = (draw_h - r.vp_h) / 2;
        r.cam_w = lw; r.cam_h = lh;
        break;
    }

    case ScaleMode::Fit:
    default: {
        const float scale = std::min(dw / lw, dh / lh);
        r.vp_w = static_cast<int>(std::lround(lw * scale));
        r.vp_h = static_cast<int>(std::lround(lh * scale));
        r.vp_x = (draw_w - r.vp_w) / 2;
        r.vp_y = (draw_h - r.vp_h) / 2;
        r.cam_w = lw; r.cam_h = lh;
        break;
    }
    }
    return r;
}

// Map a pointer position reported by the OS (in window *points* — the space
// SDL_GetMouseState uses) into logical (design) units, inverting the viewport
// mapping compute_scaling() produced. This lets screen_to_world() stay correct
// under any scale mode or HiDPI ratio.
//   r            — the ScaleResult for this frame (viewport in device pixels).
//   draw_w/draw_h — drawable surface size in device pixels.
//   point_w/point_h — current window size in points (mouse-coordinate space).
//   px/py        — the pointer position in window points.
// When no scaling/HiDPI is active this is the identity, so existing games that
// feed mouse_position() straight into screen_to_world() keep working unchanged.
inline void window_point_to_logical(const ScaleResult& r,
                                    int draw_w, int draw_h,
                                    int point_w, int point_h,
                                    float px, float py,
                                    float& out_x, float& out_y) {
    // points → device pixels (uniform HiDPI scale; 1.0 on non-HiDPI displays).
    const float sx = point_w > 0 ? static_cast<float>(draw_w) / point_w : 1.f;
    const float sy = point_h > 0 ? static_cast<float>(draw_h) / point_h : 1.f;
    const float dx = px * sx, dy = py * sy;
    // device pixels → logical units: invert the viewport rect → camera mapping.
    out_x = r.vp_w > 0 ? (dx - r.vp_x) / r.vp_w * r.cam_w : px;
    out_y = r.vp_h > 0 ? (dy - r.vp_y) / r.vp_h * r.cam_h : py;
}

} // namespace loom
