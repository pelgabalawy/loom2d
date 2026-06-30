#pragma once
#include "ui/widget.hpp"
#include "graphics/camera.hpp"
#include <memory>

namespace loom {

class Renderer;

// A screen-space UI layer: a tree of widgets laid out against a fixed logical
// rectangle and drawn through a camera that never follows the world. The run
// loop drives it each frame: set_screen() + update_input() then draw().
//
// Pointer dispatch (update_input) and focus are GPU-free and unit-tested; only
// draw() touches the Renderer. A click fires when the pointer is pressed and
// released over the same widget.
class UICanvas {
public:
    UICanvas();

    // ── Tree ────────────────────────────────────────────────────────────────
    void add(std::shared_ptr<Widget> widget);
    void remove(Widget* widget);
    void clear();
    std::shared_ptr<Widget> root_ptr() { return m_root; }
    Widget&                 root()     { return *m_root; }

    // ── Per-frame driving ───────────────────────────────────────────────────
    // The logical screen rect the UI is laid out against (origin top-left).
    void set_screen(int width, int height);
    int  screen_width()  const { return m_width;  }
    int  screen_height() const { return m_height; }

    // Re-anchor every widget against the current screen rect.
    void layout();

    // Update hover/press/focus from the pointer and fire on_click. `pressed`/
    // `released` are the edge events for this frame; `down` is the held state.
    void update_input(Vec2 pointer, bool pressed, bool down, bool released);

    void draw(Renderer& renderer);

    // ── Focus ───────────────────────────────────────────────────────────────
    void    focus(Widget* w);     // null clears focus
    void    clear_focus() { focus(nullptr); }
    Widget* focused() const { return m_focused; }
    // Move focus to the next focusable widget in tree order (wraps). Useful for
    // Tab navigation. No-op if nothing is focusable.
    void    focus_next();

    // The screen-space camera (logical units, top-left origin). The run loop
    // pushes its view-projection before drawing the UI.
    Camera camera;

private:
    void set_focus_flag(Widget* w, bool value);

    std::shared_ptr<Widget> m_root;
    int     m_width  = 0;
    int     m_height = 0;
    Widget* m_focused      = nullptr;
    Widget* m_press_target = nullptr;
    Widget* m_hovered      = nullptr;
};

} // namespace loom
