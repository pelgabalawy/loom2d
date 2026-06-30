#pragma once
#include "ui/widget.hpp"
#include "graphics/color.hpp"
#include "text/font.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace loom {

class Texture;

// Draw `text` with `font` inside `box`, honouring horizontal align and optional
// vertical centring. Shared by Label and Button. No-op without a font.
void draw_text(Renderer& renderer, Font& font, const std::string& text,
               const Rect& box, const Color& color, TextAlign align,
               bool vcenter);

// ── Panel ───────────────────────────────────────────────────────────────────
// A solid rectangle, optionally with a border. The basic UI container/backdrop.
class Panel : public Widget {
public:
    Color background   = Color(0.f, 0.f, 0.f, 0.5f);
    Color border_color = Color::white();
    float border_width = 0.f;   // 0 = no border

    Panel() = default;
    explicit Panel(Color background) : background(background) {}

    void draw(Renderer& renderer) override;
};

// ── Label ───────────────────────────────────────────────────────────────────
// A run of text. Wraps to the widget's width when size.x > 0 (0 = no wrap).
class Label : public Widget {
public:
    Color     color   = Color::white();
    TextAlign align   = TextAlign::Left;
    bool      vcenter = false;

    Label() = default;
    explicit Label(std::shared_ptr<Font> font, std::string text = "");

    void set_font(std::shared_ptr<Font> font);
    std::shared_ptr<Font> font() const { return m_font; }

    void set_text(const std::string& text);
    const std::string& text() const { return m_text; }

    void draw(Renderer& renderer) override;

private:
    std::shared_ptr<Font> m_font;
    std::string           m_text;
};

// ── Button ──────────────────────────────────────────────────────────────────
// A clickable widget: a coloured background that reacts to hover/press/disabled
// state plus a centred caption. on_click fires on press+release over the button.
class Button : public Widget {
public:
    std::string caption;
    Color text_color  = Color::white();
    Color bg          = Color(0.20f, 0.22f, 0.28f, 1.f);
    Color bg_hover    = Color(0.28f, 0.32f, 0.40f, 1.f);
    Color bg_pressed  = Color(0.14f, 0.16f, 0.20f, 1.f);
    Color bg_disabled = Color(0.16f, 0.16f, 0.18f, 1.f);

    // Called when the button is clicked (settable from Python).
    std::function<void()> on_clicked;

    Button() { focusable = true; }
    explicit Button(std::shared_ptr<Font> font, std::string caption = "");

    void set_font(std::shared_ptr<Font> font) { m_font = std::move(font); }
    std::shared_ptr<Font> font() const { return m_font; }

    void on_click() override { if (on_clicked) on_clicked(); }
    void draw(Renderer& renderer) override;

    // The background colour for the current state (exposed for testing/styling).
    Color current_background() const;

private:
    std::shared_ptr<Font> m_font;
};

// ── Image ───────────────────────────────────────────────────────────────────
// Draws a texture (optionally a source sub-rect) stretched to the widget rect.
class Image : public Widget {
public:
    Color tint = Color::white();
    Rect  source;   // sub-rect in texture pixels; zero size = whole texture

    Image() = default;
    explicit Image(std::shared_ptr<Texture> texture) : m_texture(std::move(texture)) {}

    void set_texture(std::shared_ptr<Texture> texture) { m_texture = std::move(texture); }
    std::shared_ptr<Texture> texture() const { return m_texture; }

    void draw(Renderer& renderer) override;

private:
    std::shared_ptr<Texture> m_texture;
};

// ── Grid ────────────────────────────────────────────────────────────────────
// Container that arranges its children into a `columns`-wide grid of equal
// cells (rows grow as needed). A child with no explicit size fills its cell;
// a child with a size is anchored within its cell by the normal anchored model.
class Grid : public Widget {
public:
    int  columns = 1;
    Vec2 spacing = {0.f, 0.f};   // gap between cells (px)

    Grid() = default;
    explicit Grid(int columns, Vec2 spacing = {0.f, 0.f})
        : columns(columns), spacing(spacing) {}

protected:
    void layout_children() override;
};

} // namespace loom
