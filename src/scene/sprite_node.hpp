#pragma once
#include "scene/node.hpp"
#include "scene/animation.hpp"
#include "graphics/texture.hpp"
#include "graphics/renderer.hpp"
#include <memory>
#include <unordered_map>

namespace loom {

class SpriteNode : public Node {
public:
    Color tint   = Color::white();
    Vec2  origin = {0.5f, 0.5f}; // pivot: (0,0)=top-left, (0.5,0.5)=center
    bool  flip_x = false;
    bool  flip_y = false;

    SpriteNode() = default;
    explicit SpriteNode(std::shared_ptr<Texture> texture);
    explicit SpriteNode(std::shared_ptr<Texture> texture, std::string name);

    void set_texture(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> texture() const { return m_texture; }

    // Source rect: which part of the texture to draw (full texture if zeroed)
    void set_source(Rect src) { m_source = src; }
    Rect source()             const { return m_source; }

    // Animation
    void add_animation(Animation anim);
    void play(const std::string& name);
    void stop();
    const std::string& current_animation() const { return m_current_anim; }

    void draw(Renderer& renderer, const Camera& camera) override;
    void update(float dt) override;

private:
    std::shared_ptr<Texture> m_texture;
    Rect m_source = {};

    std::unordered_map<std::string, Animation> m_animations;
    std::string m_current_anim;
};

} // namespace loom
