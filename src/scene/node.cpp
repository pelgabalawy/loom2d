#include "scene/node.hpp"
#include <algorithm>
#include <cmath>

namespace loom {

Node::Node(std::string name) : name(std::move(name)) {}

Vec2 Node::world_position() const {
    if (!m_parent) return m_position;
    Vec2  pp = m_parent->world_position();
    Vec2  ps = m_parent->world_scale();
    float pr = m_parent->world_rotation();
    Vec2 local = {m_position.x * ps.x, m_position.y * ps.y};
    return pp + local.rotated(pr);
}

float Node::world_rotation() const {
    return m_parent ? m_parent->world_rotation() + m_rotation : m_rotation;
}

Vec2 Node::world_scale() const {
    if (!m_parent) return m_scale;
    Vec2 ps = m_parent->world_scale();
    return {m_scale.x * ps.x, m_scale.y * ps.y};
}

void Node::add_child(std::shared_ptr<Node> child) {
    if (child->m_parent) child->remove_from_parent();
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

void Node::remove_child(Node* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
                           [child](const auto& p){ return p.get() == child; });
    if (it != m_children.end()) {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

void Node::remove_from_parent() {
    if (m_parent) m_parent->remove_child(this);
}

void Node::update(float dt) {
    for (auto& child : m_children) child->update(dt);
}

void Node::draw(Renderer& renderer, const Camera& camera) {
    if (!visible) return;
    for (auto& child : m_children) child->draw(renderer, camera);
}

} // namespace loom
