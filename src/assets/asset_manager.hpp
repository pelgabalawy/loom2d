#pragma once
#include "graphics/texture.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <stdexcept>

namespace loom {

// Generic ref-counted asset cache.
// Assets are loaded on first request and evicted when no one holds a reference.
template<typename T>
class AssetCache {
public:
    using Loader = std::function<std::shared_ptr<T>(const std::string&)>;

    explicit AssetCache(Loader loader) : m_loader(std::move(loader)) {}

    std::shared_ptr<T> get(const std::string& path) {
        auto it = m_cache.find(path);
        if (it != m_cache.end()) {
            if (auto ptr = it->second.lock()) return ptr;
            m_cache.erase(it); // stale weak_ptr
        }
        auto ptr = m_loader(path);
        m_cache[path] = ptr;
        return ptr;
    }

    // Force a reload even if cached
    std::shared_ptr<T> reload(const std::string& path) {
        m_cache.erase(path);
        return get(path);
    }

    void evict(const std::string& path) { m_cache.erase(path); }
    void clear()                        { m_cache.clear(); }

    int live_count() const {
        int n = 0;
        for (auto& [k, v] : m_cache) if (!v.expired()) ++n;
        return n;
    }

private:
    Loader m_loader;
    std::unordered_map<std::string, std::weak_ptr<T>> m_cache;
};

// High-level manager that wraps per-type caches.
class AssetManager {
public:
    AssetManager() = default;
    explicit AssetManager(SDL_Renderer* renderer);

    void set_renderer(SDL_Renderer* renderer);

    // Texture cache (requires renderer)
    std::shared_ptr<Texture> texture(const std::string& path);

    // Evict everything (useful before hot-reload)
    void clear();

private:
    SDL_Renderer*                   m_renderer = nullptr;
    std::unique_ptr<AssetCache<Texture>> m_textures;

    void init_caches();
};

} // namespace loom
