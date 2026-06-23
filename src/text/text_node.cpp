#include "text/text_node.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"
#include "graphics/camera.hpp"
#include <cmath>

namespace loom {

TextNode::TextNode(std::shared_ptr<Font> font, std::string text)
    : m_font(std::move(font)), m_text(std::move(text)) {
    relayout();
}

void TextNode::set_font(std::shared_ptr<Font> font) {
    m_font = std::move(font);
    relayout();
}

void TextNode::set_text(const std::string& text) {
    if (text == m_text) return;
    m_text = text;
    relayout();
}

void TextNode::set_align(TextAlign align) {
    if (align == m_align) return;
    m_align = align;
    relayout();
}

void TextNode::set_max_width(float w) {
    if (w == m_max_width) return;
    m_max_width = w;
    relayout();
}

void TextNode::relayout() {
    if (!m_font) { m_quads.clear(); m_size = {0.f, 0.f}; return; }
    m_quads = m_font->layout(m_text, m_max_width, m_align, m_size);
}

void TextNode::draw(Renderer& renderer, const Camera& camera) {
    if (!visible || !m_font || m_quads.empty()) {
        Node::draw(renderer, camera);
        return;
    }

    Vec2  wp = world_position();
    float wr = world_rotation();
    Vec2  ws = world_scale();
    float cs = std::cos(wr), sn = std::sin(wr);

    // Pivot offset within the block (pixels), so origin anchors the text.
    float ox = origin.x * m_size.x;
    float oy = origin.y * m_size.y;

    const Texture& atlas = *m_font->atlas();

    for (const GlyphQuad& g : m_quads) {
        float xs[4] = { g.x0, g.x1, g.x1, g.x0 }; // TL, TR, BR, BL
        float ys[4] = { g.y0, g.y0, g.y1, g.y1 };

        SpriteQuad q;
        for (int i = 0; i < 4; ++i) {
            float lx = (xs[i] - ox) * ws.x;
            float ly = (ys[i] - oy) * ws.y;
            q.pos[i] = { wp.x + (lx * cs - ly * sn),
                         wp.y + (lx * sn + ly * cs) };
        }
        q.uv[0][0] = g.u0; q.uv[0][1] = g.v0;
        q.uv[1][0] = g.u1; q.uv[1][1] = g.v0;
        q.uv[2][0] = g.u1; q.uv[2][1] = g.v1;
        q.uv[3][0] = g.u0; q.uv[3][1] = g.v1;

        renderer.batcher().submit(atlas, q, color);
    }

    Node::draw(renderer, camera);
}

} // namespace loom
