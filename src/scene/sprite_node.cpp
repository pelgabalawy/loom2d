#include "scene/sprite_node.hpp"
#include "graphics/camera.hpp"
#include <SDL3/SDL.h>
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

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

    SDL_Renderer* r = renderer.sdl_renderer();

    // Resolve source rect (from animation or explicit source or full texture)
    Rect src = m_source;
    if (!m_current_anim.empty()) {
        auto it = m_animations.find(m_current_anim);
        if (it != m_animations.end() && it->second.frame_count() > 0)
            src = it->second.current_frame().source;
    }

    int tex_w = m_texture->width();
    int tex_h = m_texture->height();

    SDL_FRect sdl_src = {};
    if (src.w > 0 && src.h > 0) {
        sdl_src = {src.x, src.y, src.w, src.h};
    } else {
        sdl_src = {0.f, 0.f, static_cast<float>(tex_w),
                              static_cast<float>(tex_h)};
    }

    // World transform
    Vec2  wpos   = world_position();
    float wrot   = world_rotation();
    Vec2  wscale = world_scale();

    // Screen position via camera
    Vec2 screen = camera.world_to_screen(wpos);

    float dw = sdl_src.w * std::abs(wscale.x) * camera.zoom();
    float dh = sdl_src.h * std::abs(wscale.y) * camera.zoom();

    SDL_FRect dst = {
        screen.x - origin.x * dw,
        screen.y - origin.y * dh,
        dw, dh
    };

    // Pivot for rotation (in dst-local coords)
    SDL_FPoint pivot = {origin.x * dw, origin.y * dh};

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flip_x && flip_y) flip = static_cast<SDL_FlipMode>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    else if (flip_x)      flip = SDL_FLIP_HORIZONTAL;
    else if (flip_y)      flip = SDL_FLIP_VERTICAL;

    SDL_SetTextureColorModFloat(m_texture->sdl_texture(), tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaModFloat(m_texture->sdl_texture(), tint.a);

    double deg = wrot * (180.0 / M_PI);
    SDL_RenderTextureRotated(r, m_texture->sdl_texture(),
                             &sdl_src, &dst, deg, &pivot, flip);

    Node::draw(renderer, camera);
}

} // namespace loom
