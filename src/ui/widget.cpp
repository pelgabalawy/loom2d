#include "ui/widget.hpp"
#include <algorithm>

namespace loom {

Rect compute_anchored_rect(const Rect& parent, Vec2 anchor, Vec2 pivot,
                           Vec2 offset, Vec2 size) {
    // A non-positive size component means "fill the parent dimension plus that
    // value": 0 stretches to the full parent extent, a negative value insets it
    // (e.g. -16 = parent width minus 16px). Positive values are taken literally.
    float w  = size.x > 0.f ? size.x : parent.w + size.x;
    float h  = size.y > 0.f ? size.y : parent.h + size.y;
    float ax = parent.x + anchor.x * parent.w;
    float ay = parent.y + anchor.y * parent.h;
    float x  = ax + offset.x - pivot.x * w;
    float y  = ay + offset.y - pivot.y * h;
    return {x, y, w, h};
}

Widget::Widget(std::string name) : name(std::move(name)) {}

void Widget::add_child(std::shared_ptr<Widget> child) {
    if (child->m_parent) child->remove_from_parent();
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

void Widget::remove_child(Widget* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
                           [child](const auto& p){ return p.get() == child; });
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void Widget::remove_from_parent() {
    if (m_parent) m_parent->remove_child(this);
}

void Widget::clear_children() {
    for (auto& c : m_children) c->m_parent = nullptr;
    m_children.clear();
}

void Widget::resolve_layout(const Rect& parent_rect) {
    m_rect = compute_anchored_rect(parent_rect, anchor, pivot, offset, size);
    layout_children();
}

void Widget::place(const Rect& final_rect) {
    m_rect = final_rect;
    layout_children();
}

void Widget::layout_children() {
    for (auto& child : m_children) child->resolve_layout(m_rect);
}

Widget* Widget::hit_test(Vec2 point) {
    if (!visible || !enabled) return nullptr;
    // Search topmost (last-drawn) child first so the visually-front widget wins.
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (Widget* hit = (*it)->hit_test(point)) return hit;
    }
    return m_rect.contains(point) ? this : nullptr;
}

bool Widget::is_in_subtree(const Widget* target) const {
    if (target == this) return true;
    for (const auto& child : m_children)
        if (child->is_in_subtree(target)) return true;
    return false;
}

void Widget::draw(Renderer& renderer) {
    if (!visible) return;
    draw_children(renderer);
}

void Widget::draw_children(Renderer& renderer) {
    for (auto& child : m_children) child->draw(renderer);
}

} // namespace loom
