#pragma once
#include "sokol_gfx.h"
#include "graphics/color.hpp"
#include "graphics/texture.hpp"
#include <memory>

namespace loom {

// An off-screen render target: draw a node tree into it, then use it like any
// other texture — a minimap, a security-camera feed, a portal, or the whole
// frame on its way through a screen shader.
//
// The canvas owns an RGBA8 texture plus the attachment view a render pass needs
// to write into it. It has no depth buffer (2D doesn't need one), which is why
// the batcher keeps separate pipelines for canvas passes.
class Canvas {
public:
    static std::shared_ptr<Canvas> create(int width, int height);
    ~Canvas();

    Canvas(const Canvas&)            = delete;
    Canvas& operator=(const Canvas&) = delete;

    int width()  const { return m_texture->width();  }
    int height() const { return m_texture->height(); }

    // What the canvas is cleared to at the start of each render into it.
    // Transparent by default, so a canvas composites over whatever is beneath it.
    Color clear_color = Color::transparent();

    // The canvas's contents, drawable anywhere a texture is (sprite, UI image).
    const std::shared_ptr<Texture>& texture() const { return m_texture; }

    // Replace the target with one of a new size — how a full-screen canvas keeps
    // up with a resized window. A no-op if the size is unchanged. Anything still
    // holding the old texture keeps a valid (but now detached) image.
    void resize(int width, int height);

    sg_view attachment() const { return m_attachment; }

private:
    Canvas(std::shared_ptr<Texture> texture, sg_view attachment);

    // Build the attachment view that lets a pass render into `texture`.
    static sg_view make_attachment(const Texture& texture);

    std::shared_ptr<Texture> m_texture;
    sg_view                  m_attachment = {};
};

} // namespace loom
