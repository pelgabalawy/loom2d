#include "graphics/texture.hpp"
#include <stdexcept>

// stb_image — implementation defined here only
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace loom {

Texture::Texture(sg_image image, sg_view view, int w, int h, std::string path)
    : m_image(image), m_view(view), m_width(w), m_height(h),
      m_path(std::move(path)) {}

Texture::~Texture() {
    // Textures can outlive the Renderer (e.g. held by a Python Game after the
    // game loop returns). Once sg_shutdown() has run, all GPU resources are
    // already freed, so only destroy explicitly while sokol is still alive.
    if (!sg_isvalid()) return;
    if (m_view.id  != SG_INVALID_ID) sg_destroy_view(m_view);
    if (m_image.id != SG_INVALID_ID) sg_destroy_image(m_image);
}

std::shared_ptr<Texture> Texture::load(const std::string& path) {
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) {
        throw std::runtime_error("Texture::load failed for '" + path + "': " +
                                 stbi_failure_reason());
    }

    auto tex = from_memory(data, w, h);
    stbi_image_free(data);
    tex->m_path = path;
    return tex;
}

std::shared_ptr<Texture> Texture::from_memory(const unsigned char* rgba,
                                               int width, int height) {
    sg_image_desc desc = {};
    desc.width        = width;
    desc.height       = height;
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    desc.data.mip_levels[0].ptr  = rgba;
    desc.data.mip_levels[0].size = static_cast<size_t>(width) * height * 4;

    sg_image img = sg_make_image(&desc);
    if (sg_query_image_state(img) != SG_RESOURCESTATE_VALID) {
        throw std::runtime_error("Texture::from_memory: sg_make_image failed");
    }

    // A texture view is what gets bound for sampling in the new sokol API.
    sg_view_desc vdesc = {};
    vdesc.texture.image = img;
    sg_view view = sg_make_view(&vdesc);
    if (sg_query_view_state(view) != SG_RESOURCESTATE_VALID) {
        sg_destroy_image(img);
        throw std::runtime_error("Texture::from_memory: sg_make_view failed");
    }

    return std::shared_ptr<Texture>(new Texture(img, view, width, height, ""));
}

} // namespace loom
