#include "assets/asset_manager.hpp"

namespace loom {

AssetManager::AssetManager(SDL_Renderer* renderer) : m_renderer(renderer) {
    init_caches();
}

void AssetManager::set_renderer(SDL_Renderer* renderer) {
    m_renderer = renderer;
    init_caches();
}

void AssetManager::init_caches() {
    m_textures = std::make_unique<AssetCache<Texture>>(
        [this](const std::string& path) {
            return Texture::load(m_renderer, path);
        });
}

std::shared_ptr<Texture> AssetManager::texture(const std::string& path) {
    if (!m_textures)
        throw std::runtime_error("AssetManager not initialized with a renderer");
    return m_textures->get(path);
}

void AssetManager::clear() {
    if (m_textures) m_textures->clear();
}

} // namespace loom
