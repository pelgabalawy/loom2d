#include "graphics/renderer.hpp"

namespace loom {

Renderer::Renderer(Window& window) : m_window(window) {}

void Renderer::begin_frame(const Color& clear) {
    SDL_SetRenderDrawColorFloat(m_window.sdl_renderer(),
                                clear.r, clear.g, clear.b, clear.a);
    SDL_RenderClear(m_window.sdl_renderer());
}

void Renderer::end_frame() {
    m_window.present();
}

SDL_Renderer* Renderer::sdl_renderer() const { return m_window.sdl_renderer(); }
int            Renderer::width()          const { return m_window.width();        }
int            Renderer::height()         const { return m_window.height();       }

} // namespace loom
