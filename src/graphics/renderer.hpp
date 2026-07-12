#pragma once
#include "platform/window.hpp"
#include "graphics/color.hpp"
#include "graphics/sprite_batcher.hpp"
#include "math/rect.hpp"
#include <memory>

namespace loom {

class Texture;

class Renderer {
public:
    explicit Renderer(Window& window);
    ~Renderer();

    void begin_frame(const Color& clear_color = Color::cornflower());
    void end_frame();

    // Restrict rendering to a sub-rectangle of the swapchain (device pixels,
    // top-left origin). Used for letterbox/pillarbox scaling; anything drawn
    // outside the rect is clipped. Call after begin_frame, before drawing.
    void set_viewport(int x, int y, int w, int h);

    SpriteBatcher& batcher() { return m_batcher; }

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
};

} // namespace loom
