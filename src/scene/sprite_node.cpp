#include "scene/sprite_node.hpp"
#include "graphics/camera.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"

namespace loom {

SpriteNode::SpriteNode(std::shared_ptr<Texture> texture)
    : m_texture(std::move(texture)) {}

SpriteNode::SpriteNode(std::shared_ptr<Texture> texture, std::string name)
    : Node(std::move(name)), m_texture(std::move(texture)) {}

void SpriteNode::set_texture(std::shared_ptr<Texture> texture) {
    m_texture = std::move(texture);
}

void SpriteNode::add_animation(Animation anim) {
    std::string key = anim.name;
    m_animations[key] = std::move(anim);
}

void SpriteNode::play(const std::string& name) {
    auto it = m_animations.find(name);
    if (it != m_animations.end()) {
        if (m_current_anim != name) {
            it->second.reset();
            m_current_anim = name;
        }
    }
}

void SpriteNode::stop() { m_current_anim.clear(); }

void SpriteNode::update(float dt) {
    if (!m_current_anim.empty()) {
        auto it = m_animations.find(m_current_anim);
        if (it != m_animations.end()) it->second.update(dt);
    }
    Node::update(dt);
}

void SpriteNode::draw(Renderer& renderer, const Camera& camera) {
    if (!visible || !m_texture) {
        Node::draw(renderer, camera);
        return;
    }

    // Resolve source rect (from animation or explicit source or full texture)
    Rect src = m_source;
    if (!m_current_anim.empty()) {
        auto it = m_animations.find(m_current_anim);
        if (it != m_animations.end() && it->second.frame_count() > 0)
            src = it->second.current_frame().source;
    }

    // Build the world-space quad (camera zoom/rotation handled by the GPU
    // view-projection matrix) and hand it to the batcher.
    SpriteQuad quad = build_sprite_quad(
        world_position(), world_rotation(), world_scale(), origin,
        src, m_texture->width(), m_texture->height(), flip_x, flip_y);

    renderer.batcher().submit(*m_texture, quad, tint);

    Node::draw(renderer, camera);
}

} // namespace loom
