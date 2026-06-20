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

    m_window = SDL_CreateWindow(title.c_str(), width, height,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window) {
        throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        SDL_DestroyWindow(m_window);
        throw std::runtime_error(std::string("SDL_CreateRenderer: ") + SDL_GetError());
    }

    SDL_SetRenderVSync(m_renderer, 1);
}

Window::~Window() {
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window)   SDL_DestroyWindow(m_window);
    SDL_Quit();
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
    SDL_RenderPresent(m_renderer);
}

} // namespace loom
