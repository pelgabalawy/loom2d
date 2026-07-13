#include "graphics/canvas.hpp"
#include <stdexcept>

namespace loom {

Canvas::Canvas(std::shared_ptr<Texture> texture, sg_view attachment)
    : m_texture(std::move(texture)), m_attachment(attachment) {}

Canvas::~Canvas() {
    // Like Texture, a Canvas can outlive the Renderer; sg_shutdown() has already
    // freed the GPU side by then.
    if (sg_isvalid() && m_attachment.id != SG_INVALID_ID) {
        sg_destroy_view(m_attachment);
    }
}

sg_view Canvas::make_attachment(const Texture& texture) {
    sg_view_desc vdesc = {};
    vdesc.color_attachment.image = texture.image();
    sg_view view = sg_make_view(&vdesc);
    if (sg_query_view_state(view) != SG_RESOURCESTATE_VALID) {
        throw std::runtime_error("Canvas: could not create the colour attachment");
    }
    return view;
}

std::shared_ptr<Canvas> Canvas::create(int width, int height) {
    if (!sg_isvalid()) {
        throw std::runtime_error(
            "Canvas: no renderer yet — create canvases in on_start() or later, "
            "not before the game window exists");
    }
    auto texture = Texture::render_target(width, height);
    sg_view attachment = make_attachment(*texture);
    return std::shared_ptr<Canvas>(new Canvas(std::move(texture), attachment));
}

void Canvas::resize(int width, int height) {
    if (width == this->width() && height == this->height()) return;

    auto texture = Texture::render_target(width, height);
    sg_view attachment = make_attachment(*texture);

    if (m_attachment.id != SG_INVALID_ID) sg_destroy_view(m_attachment);
    m_texture    = std::move(texture); // the old image dies with its last owner
    m_attachment = attachment;
}

} // namespace loom
