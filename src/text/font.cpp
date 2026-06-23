#include "text/font.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <vector>

// stb_truetype — implementation defined here only (stb_image's impl lives in
// texture.cpp; they are separate headers so there is no collision).
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace loom {

// ── Pure word-wrap (unit-tested, no GPU) ─────────────────────────────────────

std::vector<std::pair<size_t, size_t>>
Font::wrap_lines(const std::string& text, const std::vector<float>& advances,
                 float max_width) {
    std::vector<std::pair<size_t, size_t>> lines;
    const size_t n = text.size();

    size_t line_start = 0;
    size_t last_space = std::string::npos; // last ' ' seen on the current line
    float  width      = 0.f;

    for (size_t i = 0; i < n; ++i) {
        if (text[i] == '\n') {
            lines.emplace_back(line_start, i);
            line_start = i + 1;
            last_space = std::string::npos;
            width = 0.f;
            continue;
        }

        width += (i < advances.size()) ? advances[i] : 0.f;
        if (text[i] == ' ') last_space = i;

        // Overflow: break at the last space if there is a real word before it.
        if (max_width > 0.f && width > max_width &&
            last_space != std::string::npos && last_space > line_start) {
            lines.emplace_back(line_start, last_space);
            line_start = last_space + 1;
            last_space = std::string::npos;
            width = 0.f;
            for (size_t j = line_start; j <= i; ++j)
                width += (j < advances.size()) ? advances[j] : 0.f;
        }
    }
    lines.emplace_back(line_start, n);
    return lines;
}

// ── Glyph access ─────────────────────────────────────────────────────────────

const Font::Glyph* Font::glyph_of(char c) const {
    int idx = static_cast<unsigned char>(c) - m_first;
    if (idx < 0 || idx >= m_count) return nullptr;
    return &m_glyphs[idx];
}

float Font::advance_of(char c) const {
    const Glyph* g = glyph_of(c);
    return g ? g->xadvance : 0.f;
}

// ── Baking ───────────────────────────────────────────────────────────────────

std::shared_ptr<Font> Font::load(const std::string& path, float pixel_height) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Font::load: cannot open '" + path + "'");
    std::vector<unsigned char> ttf((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
    if (ttf.empty())
        throw std::runtime_error("Font::load: empty file '" + path + "'");

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf.data(),
                        stbtt_GetFontOffsetForIndex(ttf.data(), 0)))
        throw std::runtime_error("Font::load: not a valid TTF '" + path + "'");

    // Atlas size grows with the requested pixel height (95 ASCII glyphs).
    int atlas_w = pixel_height <= 40.f ? 512 : 1024;
    int atlas_h = atlas_w;

    auto font = std::shared_ptr<Font>(new Font());
    font->m_pixel_height = pixel_height;
    font->m_atlas_w = atlas_w;
    font->m_atlas_h = atlas_h;

    std::vector<unsigned char> alpha(static_cast<size_t>(atlas_w) * atlas_h, 0);
    std::vector<stbtt_bakedchar> cdata(font->m_count);
    int res = stbtt_BakeFontBitmap(ttf.data(), 0, pixel_height, alpha.data(),
                                   atlas_w, atlas_h, font->m_first,
                                   font->m_count, cdata.data());
    if (res == 0)
        throw std::runtime_error("Font::load: failed to bake atlas for '" +
                                 path + "' (atlas too small?)");

    // Copy stb's baked metrics into our glyph table.
    font->m_glyphs.resize(font->m_count);
    for (int i = 0; i < font->m_count; ++i) {
        const stbtt_bakedchar& b = cdata[i];
        Glyph& g = font->m_glyphs[i];
        g.ax0 = b.x0; g.ay0 = b.y0; g.ax1 = b.x1; g.ay1 = b.y1;
        g.xoff = b.xoff; g.yoff = b.yoff; g.xadvance = b.xadvance;
    }

    // Vertical metrics in pixels.
    float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    font->m_ascent      = ascent * scale;
    font->m_line_height = (ascent - descent + line_gap) * scale;

    // Expand the 8-bit coverage atlas to white RGBA with alpha = coverage, so
    // the existing sprite shader (texture * tint) renders tinted, anti-aliased
    // glyphs without a dedicated text shader.
    std::vector<unsigned char> rgba(static_cast<size_t>(atlas_w) * atlas_h * 4);
    for (size_t i = 0; i < alpha.size(); ++i) {
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = alpha[i];
    }
    font->m_atlas = Texture::from_memory(rgba.data(), atlas_w, atlas_h);

    return font;
}

// ── Layout ───────────────────────────────────────────────────────────────────

std::vector<GlyphQuad> Font::layout(const std::string& text, float max_width,
                                    TextAlign align, Vec2& out_size) const {
    std::vector<float> adv(text.size());
    for (size_t i = 0; i < text.size(); ++i)
        adv[i] = (text[i] == '\n') ? 0.f : advance_of(text[i]);

    auto lines = wrap_lines(text, adv, max_width);

    // Block width = widest line.
    float block_w = 0.f;
    for (auto& ln : lines) {
        float lw = 0.f;
        for (size_t j = ln.first; j < ln.second; ++j) lw += adv[j];
        block_w = std::max(block_w, lw);
    }
    float block_h = lines.size() * m_line_height;

    std::vector<GlyphQuad> quads;
    for (size_t li = 0; li < lines.size(); ++li) {
        size_t s = lines[li].first, e = lines[li].second;
        float lw = 0.f;
        for (size_t j = s; j < e; ++j) lw += adv[j];

        float x = 0.f;
        if (align == TextAlign::Center)     x = (block_w - lw) * 0.5f;
        else if (align == TextAlign::Right) x = (block_w - lw);

        float baseline = m_ascent + li * m_line_height;

        for (size_t j = s; j < e; ++j) {
            const Glyph* g = glyph_of(text[j]);
            if (g && g->ax1 > g->ax0) { // skip glyphs with no bitmap (e.g. space)
                GlyphQuad q;
                q.x0 = x + g->xoff;
                q.y0 = baseline + g->yoff;
                q.x1 = q.x0 + (g->ax1 - g->ax0);
                q.y1 = q.y0 + (g->ay1 - g->ay0);
                q.u0 = g->ax0 / static_cast<float>(m_atlas_w);
                q.u1 = g->ax1 / static_cast<float>(m_atlas_w);
                q.v0 = g->ay0 / static_cast<float>(m_atlas_h);
                q.v1 = g->ay1 / static_cast<float>(m_atlas_h);
                quads.push_back(q);
            }
            x += adv[j];
        }
    }

    out_size = { block_w, block_h };
    return quads;
}

Vec2 Font::measure(const std::string& text, float max_width) const {
    Vec2 size;
    layout(text, max_width, TextAlign::Left, size);
    return size;
}

} // namespace loom
