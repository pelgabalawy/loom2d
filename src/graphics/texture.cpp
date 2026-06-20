#include "graphics/texture.hpp"
#include <stdexcept>

// stb_image — implementation defined here only
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace loom {

Texture::Texture(SDL_Texture* tex, int w, int h, std::string path)
    : m_texture(tex), m_width(w), m_height(h), m_path(std::move(path)) {}

Texture::~Texture() {
    if (m_texture) SDL_DestroyTexture(m_texture);
}

std::shared_ptr<Texture> Texture::load(SDL_Renderer* renderer,
                                        const std::string& path)
{
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) {
        throw std::runtime_error("Texture::load failed for '" + path + "': " +
                                 stbi_failure_reason());
    }

    auto tex = from_memory(renderer, data, w, h);
    stbi_image_free(data);
    tex->m_path = path;
    return tex;
}

std::shared_ptr<Texture> Texture::from_memory(SDL_Renderer* renderer,
                                               const unsigned char* rgba,
                                               int width, int height)
{
    SDL_Surface* surf = SDL_CreateSurfaceFrom(
        width, height, SDL_PIXELFORMAT_RGBA32,
        const_cast<unsigned char*>(rgba), width * 4);
    if (!surf) {
        throw std::runtime_error(std::string("SDL_CreateSurfaceFrom: ") +
                                 SDL_GetError());
    }

    SDL_Texture* sdl_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    if (!sdl_tex) {
        throw std::runtime_error(std::string("SDL_CreateTextureFromSurface: ") +
                                 SDL_GetError());
    }

    SDL_SetTextureBlendMode(sdl_tex, SDL_BLENDMODE_BLEND);
    return std::shared_ptr<Texture>(new Texture(sdl_tex, width, height, ""));
}

} // namespace loom
