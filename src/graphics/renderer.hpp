#pragma once
#include "platform/window.hpp"
#include "graphics/color.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/sprite_batcher.hpp"
#include "math/rect.hpp"
#include <memory>

namespace loom {

class Texture;
class Canvas;
class Shader;

class Renderer {
public:
    explicit Renderer(Window& window);
    ~Renderer();

    // ── Frame & passes ──────────────────────────────────────────────────────
    // A frame is one or more passes. Each pass targets either the window or a
    // canvas, and everything drawn between begin_pass and end_pass goes there.
    // Passes cannot nest: render your canvases first, then the window.
    void begin_frame();
    void end_frame();

    // target == nullptr draws to the window.
    void begin_pass(Canvas* target, const Color& clear_color);
    void end_pass();
    bool in_pass() const { return m_in_pass; }

    // Restrict rendering to a sub-rectangle of the target (device pixels,
    // top-left origin). Used for letterbox/pillarbox scaling; anything drawn
    // outside the rect is clipped. Call inside a pass, before drawing.
    void set_viewport(int x, int y, int w, int h);

    SpriteBatcher& batcher() { return m_batcher; }

    // ── Draw state ──────────────────────────────────────────────────────────
    // Every drawable sets these at the top of its draw(), so state never leaks
    // between drawables. nullptr = the built-in sprite shader.
    void set_shader(const std::shared_ptr<Shader>& shader) { m_batcher.set_shader(shader); }
    void set_blend(BlendMode blend)                        { m_batcher.set_blend(blend); }

    // ── Immediate-mode quad helpers (used by the UI layer) ──────────────────
    // Submit an axis-aligned, screen-space rectangle to the current batch.
    // Coordinates are interpreted by whatever view-projection is currently set
    // on the batcher. fill_rect draws a solid colour via a shared 1x1 white
    // texture; draw_texture maps a texture (optional source sub-rect) onto dst.
    void fill_rect(const Rect& dst, const Color& color);
    void draw_texture(const Texture& texture, const Rect& dst,
                      const Color& tint = Color::white(), Rect src = {});

    // A shared 1x1 opaque-white texture, created lazily. Tinting it yields a
    // solid colour quad through the existing sprite shader.
    const Texture& white_texture();

    int width()  const;
    int height() const;

private:
    Window&                  m_window;
    SpriteBatcher            m_batcher;
    std::shared_ptr<Texture> m_white;
    bool                     m_in_pass = false;
};

} // namespace loom
