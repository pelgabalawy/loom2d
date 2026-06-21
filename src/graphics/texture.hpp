#pragma once
#include "sokol_gfx.h"
#include <string>
#include <memory>

namespace loom {

class Texture {
public:
    // Load from a file path using stb_image. Requires sokol_gfx to be set up
    // (i.e. a Renderer must exist).
    static std::shared_ptr<Texture> load(const std::string& path);

    // Create from raw RGBA bytes (for atlas building / tests)
    static std::shared_ptr<Texture> from_memory(const unsigned char* rgba,
                                                 int width, int height);

    ~Texture();

    // Non-copyable, movable
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    sg_image           image()  const { return m_image;  }
    sg_view            view()   const { return m_view;   }  // texture view for binding
    int                width()  const { return m_width;  }
    int                height() const { return m_height; }
    const std::string& path()   const { return m_path;   }

private:
    Texture(sg_image image, sg_view view, int w, int h, std::string path);

    sg_image    m_image  = {};
    sg_view     m_view   = {};
    int         m_width  = 0;
    int         m_height = 0;
    std::string m_path;
};

} // namespace loom
