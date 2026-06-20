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

    void present();

    SDL_Window*   sdl_window()   const { return m_window;   }
    SDL_Renderer* sdl_renderer() const { return m_renderer; }

    int width()  const { return m_width;  }
    int height() const { return m_height; }

private:
    SDL_Window*   m_window   = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    int m_width, m_height;
};

} // namespace loom
