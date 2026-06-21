#include "assets/asset_manager.hpp"

namespace loom {

void AssetManager::init_caches() {
    m_textures = std::make_unique<AssetCache<Texture>>(
        [](const std::string& path) {
            return Texture::load(path);
        });
}

std::shared_ptr<Texture> AssetManager::texture(const std::string& path) {
    if (!m_textures)
        throw std::runtime_error("AssetManager not initialized");
    return m_textures->get(path);
}

void AssetManager::clear() {
    if (m_textures) m_textures->clear();
}

} // namespace loom
