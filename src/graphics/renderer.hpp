#pragma once
#include "platform/window.hpp"
#include "graphics/color.hpp"
#include "graphics/sprite_batcher.hpp"

namespace loom {

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

    int width()  const;
    int height() const;

private:
    Window&       m_window;
    SpriteBatcher m_batcher;
};

} // namespace loom
