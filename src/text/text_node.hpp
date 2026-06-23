#pragma once
#include "scene/node.hpp"
#include "text/font.hpp"
#include "graphics/color.hpp"
#include <memory>
#include <string>
#include <vector>

namespace loom {

// A scene node that draws a string with a Font. The glyphs share the font's
// single atlas, so a whole text block is one draw call. Re-layout happens lazily
// whenever the text/font/wrap/align change.
class TextNode : public Node {
public:
    Color color  = Color::white();
    // Block anchor (0,0)=top-left, (0.5,0.5)=center, (1,1)=bottom-right.
    Vec2  origin = {0.f, 0.f};

    TextNode() = default;
    explicit TextNode(std::shared_ptr<Font> font, std::string text = "");

    void set_font(std::shared_ptr<Font> font);
    std::shared_ptr<Font> font() const { return m_font; }

    void set_text(const std::string& text);
    const std::string& text() const { return m_text; }

    void set_align(TextAlign align);
    TextAlign align() const { return m_align; }

    // Wrap width in pixels (pre-scale); 0 = no wrapping (only '\n' breaks).
    void set_max_width(float w);
    float max_width() const { return m_max_width; }

    // Laid-out block size in pixels (before node scale).
    Vec2 size() const { return m_size; }

    void draw(Renderer& renderer, const Camera& camera) override;

private:
    void relayout();

    std::shared_ptr<Font>  m_font;
    std::string            m_text;
    TextAlign              m_align     = TextAlign::Left;
    float                  m_max_width = 0.f;
    std::vector<GlyphQuad> m_quads;
    Vec2                   m_size = {0.f, 0.f};
};

} // namespace loom
