#include "ui/widgets.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"
#include "graphics/texture.hpp"
#include <algorithm>
#include <cmath>

namespace loom {

// ── Shared text rendering ───────────────────────────────────────────────────

void draw_text(Renderer& renderer, Font& font, const std::string& text,
               const Rect& box, const Color& color, TextAlign align,
               bool vcenter) {
    if (text.empty()) return;

    Vec2 size;
    // Wrap to the box width when it has one; layout also aligns each line within
    // the block. We then position the whole block within the box below.
    std::vector<GlyphQuad> quads =
        font.layout(text, box.w > 0.f ? box.w : 0.f, align, size);
    if (quads.empty()) return;

    float dx = box.x;
    if (box.w > 0.f) {
        if (align == TextAlign::Center)     dx += (box.w - size.x) * 0.5f;
        else if (align == TextAlign::Right) dx += (box.w - size.x);
    }
    float dy = box.y + (vcenter ? (box.h - size.y) * 0.5f : 0.f);

    const Texture& atlas = *font.atlas();
    SpriteBatcher& batcher = renderer.batcher();
    for (const GlyphQuad& g : quads) {
        SpriteQuad q;
        q.pos[0] = {dx + g.x0, dy + g.y0};
        q.pos[1] = {dx + g.x1, dy + g.y0};
        q.pos[2] = {dx + g.x1, dy + g.y1};
        q.pos[3] = {dx + g.x0, dy + g.y1};
        q.uv[0][0] = g.u0; q.uv[0][1] = g.v0;
        q.uv[1][0] = g.u1; q.uv[1][1] = g.v0;
        q.uv[2][0] = g.u1; q.uv[2][1] = g.v1;
        q.uv[3][0] = g.u0; q.uv[3][1] = g.v1;
        batcher.submit(atlas, q, color);
    }
}

// ── Panel ───────────────────────────────────────────────────────────────────

void Panel::draw(Renderer& renderer) {
    if (!visible) return;
    if (background.a > 0.f) renderer.fill_rect(m_rect, background);

    float bw = border_width;
    if (bw > 0.f) {
        const Rect& r = m_rect;
        renderer.fill_rect({r.x,            r.y,            r.w,          bw},          border_color); // top
        renderer.fill_rect({r.x,            r.y + r.h - bw, r.w,          bw},          border_color); // bottom
        renderer.fill_rect({r.x,            r.y + bw,       bw,           r.h - 2 * bw}, border_color); // left
        renderer.fill_rect({r.x + r.w - bw, r.y + bw,       bw,           r.h - 2 * bw}, border_color); // right
    }
    draw_children(renderer);
}

// ── Label ───────────────────────────────────────────────────────────────────

Label::Label(std::shared_ptr<Font> font, std::string text)
    : m_font(std::move(font)), m_text(std::move(text)) {}

void Label::set_font(std::shared_ptr<Font> font) { m_font = std::move(font); }
void Label::set_text(const std::string& text)    { m_text = text; }

void Label::draw(Renderer& renderer) {
    if (!visible) return;
    if (m_font) draw_text(renderer, *m_font, m_text, m_rect, color, align, vcenter);
    draw_children(renderer);
}

// ── Button ──────────────────────────────────────────────────────────────────

Button::Button(std::shared_ptr<Font> font, std::string caption)
    : caption(std::move(caption)), m_font(std::move(font)) { focusable = true; }

Color Button::current_background() const {
    if (!enabled) return bg_disabled;
    if (pressed)  return bg_pressed;
    if (hovered)  return bg_hover;
    return bg;
}

void Button::draw(Renderer& renderer) {
    if (!visible) return;
    renderer.fill_rect(m_rect, current_background());
    if (m_font)
        draw_text(renderer, *m_font, caption, m_rect, text_color,
                  TextAlign::Center, /*vcenter=*/true);
    draw_children(renderer);
}

// ── Image ───────────────────────────────────────────────────────────────────

void Image::draw(Renderer& renderer) {
    if (!visible) return;
    if (m_texture) renderer.draw_texture(*m_texture, m_rect, tint, source);
    draw_children(renderer);
}

// ── Grid ────────────────────────────────────────────────────────────────────

void Grid::layout_children() {
    const auto& kids = children();
    if (kids.empty()) return;

    int cols = std::max(1, columns);
    int rows = static_cast<int>((kids.size() + cols - 1) / cols); // ceil
    float cw = (m_rect.w - (cols - 1) * spacing.x) / cols;
    float ch = (m_rect.h - (rows - 1) * spacing.y) / rows;

    for (size_t i = 0; i < kids.size(); ++i) {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;
        Rect cell{ m_rect.x + col * (cw + spacing.x),
                   m_rect.y + row * (ch + spacing.y), cw, ch };

        // The anchored model fills the cell when a child has no explicit size
        // (size <= 0), or places it within the cell when it does.
        kids[i]->resolve_layout(cell);
    }
}

} // namespace loom
