#pragma once
#include <string>
#include <SDL3/SDL.h>

namespace loom {

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    // Returns false when the user closes the window or presses Escape
    bool poll_events();

    // Swap the GL back buffer to the screen
    void present();

    SDL_Window*   sdl_window()   const { return m_window;  }
    SDL_GLContext gl_context()   const { return m_gl;      }

    // Logical (point) size
    int width()  const { return m_width;  }
    int height() const { return m_height; }

    // Backing-store size in pixels (may exceed logical size on HiDPI displays).
    // This is what the GL viewport / swapchain must use.
    int drawable_width()  const;
    int drawable_height() const;

private:
    SDL_Window*   m_window = nullptr;
    SDL_GLContext m_gl     = nullptr;
    int m_width, m_height;
};

} // namespace loom
