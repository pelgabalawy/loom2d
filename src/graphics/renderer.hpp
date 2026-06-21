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

    SpriteBatcher& batcher() { return m_batcher; }

    int width()  const;
    int height() const;

private:
    Window&       m_window;
    SpriteBatcher m_batcher;
};

} // namespace loom
