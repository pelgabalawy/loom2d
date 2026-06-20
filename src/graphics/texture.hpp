#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <memory>

namespace loom {

class Texture {
public:
    // Load from a file path using stb_image
    static std::shared_ptr<Texture> load(SDL_Renderer* renderer,
                                         const std::string& path);

    // Create from raw RGBA bytes (for atlas building / tests)
    static std::shared_ptr<Texture> from_memory(SDL_Renderer* renderer,
                                                 const unsigned char* rgba,
                                                 int width, int height);

    ~Texture();

    // Non-copyable, movable
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    SDL_Texture* sdl_texture() const { return m_texture; }
    int          width()       const { return m_width;   }
    int          height()      const { return m_height;  }
    const std::string& path()  const { return m_path;    }

private:
    explicit Texture(SDL_Texture* tex, int w, int h, std::string path);

    SDL_Texture* m_texture = nullptr;
    int          m_width   = 0;
    int          m_height  = 0;
    std::string  m_path;
};

} // namespace loom
