#include "platform/window.hpp"
#include <stdexcept>
#include <string>

namespace loom {

Window::Window(const std::string& title, int width, int height)
    : m_width(width), m_height(height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
    }

    // Request the GL context sokol_gfx expects before creating the window.
#if defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_window = SDL_CreateWindow(title.c_str(), width, height,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
    }

    m_gl = SDL_GL_CreateContext(m_window);
    if (!m_gl) {
        SDL_DestroyWindow(m_window);
        throw std::runtime_error(std::string("SDL_GL_CreateContext: ") + SDL_GetError());
    }

    SDL_GL_MakeCurrent(m_window, m_gl);
    // vsync on by default; set LOOM2D_NO_VSYNC=1 to uncap (benchmarking).
    SDL_GL_SetSwapInterval(SDL_getenv("LOOM2D_NO_VSYNC") ? 0 : 1);
}

Window::~Window() {
    if (m_gl)     SDL_GL_DestroyContext(m_gl);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

int Window::drawable_width() const {
    int w = m_width, h = m_height;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return w;
}

int Window::drawable_height() const {
    int w = m_width, h = m_height;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return h;
}

bool Window::poll_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) return false;
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) return false;
    }
    return true;
}

void Window::present() {
    SDL_GL_SwapWindow(m_window);
}

} // namespace loom
