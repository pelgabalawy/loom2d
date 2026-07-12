#pragma once
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include <memory>
#include <string>
#include <vector>

namespace loom {

class Renderer;

// ── Anchored layout (pure, unit-tested) ─────────────────────────────────────
// A widget is positioned relative to its parent's rect:
//   anchor — a point in the PARENT (0,0 = top-left .. 1,1 = bottom-right)
//   pivot  — the point in the WIDGET that lands on the anchor (same 0..1 space)
//   offset — a pixel nudge applied after anchoring
//   size   — the widget's size in logical pixels; a non-positive component means
//            "fill the parent dimension plus that value" (0 = stretch to fill,
//            -16 = parent minus 16px), so omitting size fills the parent.
// Defaults (anchor 0,0 / pivot 0,0) place the widget at the parent's top-left
// plus offset. anchor & pivot of (0.5,0.5) centres it; (1,1)/(1,1) pins it to
// the bottom-right with `offset` acting as an inset.
Rect compute_anchored_rect(const Rect& parent, Vec2 anchor, Vec2 pivot,
                           Vec2 offset, Vec2 size);

// Base class for all UI widgets. Widgets live in a screen-space tree rooted at a
// UICanvas and are laid out with the anchored model above — independent of the
// world camera. Layout, hit-testing and pointer dispatch are GPU-free so they
// can be unit-tested headlessly; only draw() needs a live Renderer.
class Widget {
public:
    std::string name;
    bool        visible   = true;   // hidden widgets neither draw nor hit-test
    bool        enabled   = true;   // disabled widgets draw but ignore input
    bool        focusable = false;  // can receive keyboard focus on click

    // Layout inputs (see compute_anchored_rect).
    Vec2 anchor = {0.f, 0.f};
    Vec2 pivot  = {0.f, 0.f};
    Vec2 offset = {0.f, 0.f};
    Vec2 size   = {0.f, 0.f};

    // Pointer/focus state — refreshed each frame by UICanvas::update_input.
    bool hovered = false;
    bool pressed = false;
    bool focused = false;

    Widget() = default;
    explicit Widget(std::string name);
    virtual ~Widget() = default;

    // ── Scene graph ─────────────────────────────────────────────────────────
    void add_child(std::shared_ptr<Widget> child);
    void remove_child(Widget* child);
    void remove_from_parent();
    void clear_children();

    Widget*                                     parent()   const { return m_parent; }
    const std::vector<std::shared_ptr<Widget>>& children() const { return m_children; }

    // Resolved absolute rect (valid after resolve_layout).
    Rect rect() const { return m_rect; }

    // ── Layout ──────────────────────────────────────────────────────────────
    // Compute this widget's absolute rect from the parent rect, then lay out
    // children. resolve_layout is non-virtual (it caches m_rect); subclasses
    // customise child placement by overriding layout_children().
    void resolve_layout(const Rect& parent_rect);

    // Force this widget to occupy exactly `final_rect`, ignoring the anchored
    // model, then lay out children within it. Used by container layouts (Grid)
    // that compute child rects themselves.
    void place(const Rect& final_rect);

    // ── Hit testing ─────────────────────────────────────────────────────────
    // Deepest visible+enabled widget whose rect contains `point`, searching
    // topmost child first (later children paint on top). null if none.
    Widget* hit_test(Vec2 point);

    // True if `target` is this widget or anywhere in its subtree. Compares
    // addresses only (never dereferences `target`), so it is safe to call with
    // a possibly-stale pointer to validate cached hover/press/focus targets.
    bool is_in_subtree(const Widget* target) const;

    // ── Drawing ─────────────────────────────────────────────────────────────
    // Default draws nothing of its own and recurses into children. Subclasses
    // draw their own visuals first, then call draw_children().
    virtual void draw(Renderer& renderer);

    // ── Events (overridable; fired by UICanvas) ─────────────────────────────
    virtual void on_click() {}

protected:
    void draw_children(Renderer& renderer);

    // Override to place children differently (e.g. Grid). Default: each child is
    // anchored within this widget's rect.
    virtual void layout_children();

    Rect                                 m_rect;
    Widget*                              m_parent = nullptr;
    std::vector<std::shared_ptr<Widget>> m_children;
};

} // namespace loom
