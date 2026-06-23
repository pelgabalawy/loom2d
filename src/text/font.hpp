#pragma once
#include "graphics/texture.hpp"
#include "math/vec2.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace loom {

enum class TextAlign { Left, Center, Right };

// One laid-out glyph: a pixel-space rectangle (relative to the text block's
// top-left, y-down) plus its atlas UVs. TextNode turns these into world quads.
struct GlyphQuad {
    float x0, y0, x1, y1;
    float u0, v0, u1, v1;
};

// A bitmap font: a TTF/OTF rasterized into one GPU atlas texture (ASCII 32..126).
// Many glyphs share the single atlas, so a whole string is one draw call.
class Font {
public:
    // Load and bake at the given pixel height. Requires a Renderer (uploads the
    // atlas to the GPU). Throws std::runtime_error on failure.
    static std::shared_ptr<Font> load(const std::string& path, float pixel_height);

    std::shared_ptr<Texture> atlas()        const { return m_atlas; }
    float                    pixel_height() const { return m_pixel_height; }
    float                    line_height()  const { return m_line_height; }
    float                    ascent()       const { return m_ascent; }

    // Lay out text into glyph quads. Handles '\n' and optional word-wrap at
    // max_width pixels (0 = no wrap). out_size receives the block's w/h.
    std::vector<GlyphQuad> layout(const std::string& text, float max_width,
                                  TextAlign align, Vec2& out_size) const;

    // Block size of the text without rendering it.
    Vec2 measure(const std::string& text, float max_width = 0.f) const;

    // Pure helper (no GPU): greedy word-wrap. Given the advance width of each
    // character in `text`, returns [start,end) byte ranges for each visual line,
    // breaking on spaces when a line would exceed max_width (0 = only on '\n').
    static std::vector<std::pair<size_t, size_t>>
    wrap_lines(const std::string& text, const std::vector<float>& advances,
               float max_width);

private:
    struct Glyph {
        std::uint16_t ax0, ay0, ax1, ay1; // atlas pixel rect
        float xoff, yoff, xadvance;
    };

    float advance_of(char c) const;
    const Glyph* glyph_of(char c) const;

    std::vector<Glyph>       m_glyphs;     // indexed by (char - m_first)
    std::shared_ptr<Texture> m_atlas;
    int   m_atlas_w = 0, m_atlas_h = 0;
    int   m_first = 32, m_count = 95;
    float m_pixel_height = 0.f, m_line_height = 0.f, m_ascent = 0.f;
};

} // namespace loom
