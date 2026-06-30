#include "ui/ui_canvas.hpp"
#include <vector>

namespace loom {

UICanvas::UICanvas() : m_root(std::make_shared<Widget>("ui_root")) {}

void UICanvas::add(std::shared_ptr<Widget> widget) {
    m_root->add_child(std::move(widget));
}

void UICanvas::remove(Widget* widget) {
    // Drop any cached pointer into the subtree about to be removed so we never
    // compare against freed memory next frame.
    if (widget && widget->is_in_subtree(m_focused))      set_focus_flag(m_focused, false), m_focused = nullptr;
    if (widget && widget->is_in_subtree(m_press_target)) m_press_target = nullptr;
    if (widget && widget->is_in_subtree(m_hovered))      m_hovered = nullptr;
    m_root->remove_child(widget);
}

void UICanvas::clear() {
    m_focused = m_press_target = m_hovered = nullptr;
    m_root->clear_children();
}

void UICanvas::set_screen(int width, int height) {
    m_width  = width;
    m_height = height;
    camera.set_viewport(width, height);
    camera.set_position(Vec2(width * 0.5f, height * 0.5f));
}

void UICanvas::layout() {
    // The root fills the whole logical screen (its own anchor/size are ignored),
    // so top-level widgets anchor against the full screen rect.
    m_root->place(Rect(0.f, 0.f,
                       static_cast<float>(m_width),
                       static_cast<float>(m_height)));
}

void UICanvas::set_focus_flag(Widget* w, bool value) {
    if (w) w->focused = value;
}

void UICanvas::focus(Widget* w) {
    if (w == m_focused) return;
    set_focus_flag(m_focused, false);
    m_focused = (w && w->focusable) ? w : nullptr;
    set_focus_flag(m_focused, true);
}

void UICanvas::update_input(Vec2 pointer, bool pressed, bool down, bool released) {
    // Validate cached targets against the live tree (a widget may have been
    // removed since last frame). is_in_subtree never dereferences the pointer.
    if (m_focused      && !m_root->is_in_subtree(m_focused))      m_focused = nullptr;
    if (m_press_target && !m_root->is_in_subtree(m_press_target)) m_press_target = nullptr;
    if (m_hovered      && !m_root->is_in_subtree(m_hovered))      m_hovered = nullptr;

    Widget* over = m_root->hit_test(pointer);

    // Hover: only the widget directly under the pointer is "hovered".
    if (m_hovered != over) {
        if (m_hovered) m_hovered->hovered = false;
        m_hovered = over;
        if (m_hovered) m_hovered->hovered = true;
    }

    if (pressed) {
        m_press_target = over;
        // Clicking sets focus (to a focusable target, else clears it).
        focus(over);
    }

    // A widget shows "pressed" only while the button is held AND the pointer is
    // still over the original press target.
    if (m_press_target)
        m_press_target->pressed = down && (over == m_press_target);

    if (released) {
        if (m_press_target && over == m_press_target)
            m_press_target->on_click();
        if (m_press_target) m_press_target->pressed = false;
        m_press_target = nullptr;
    }
}

void UICanvas::draw(Renderer& renderer) {
    m_root->draw(renderer);
}

namespace {
// Flatten focusable widgets in depth-first (draw) order.
void collect_focusable(Widget* w, std::vector<Widget*>& out) {
    if (w->focusable && w->visible && w->enabled) out.push_back(w);
    for (const auto& c : w->children()) collect_focusable(c.get(), out);
}
} // namespace

void UICanvas::focus_next() {
    std::vector<Widget*> order;
    collect_focusable(m_root.get(), order);
    if (order.empty()) { focus(nullptr); return; }

    int cur = -1;
    for (size_t i = 0; i < order.size(); ++i)
        if (order[i] == m_focused) { cur = static_cast<int>(i); break; }

    focus(order[(cur + 1) % order.size()]);
}

} // namespace loom
