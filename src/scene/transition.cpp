#include "scene/transition.hpp"
#include "graphics/renderer.hpp"
#include "math/rect.hpp"
#include <algorithm>

namespace loom {

Fade::Fade(float duration, Color color)
    : duration(duration), color(color) {}

void Fade::update(float dt) {
    m_elapsed += dt;
}

// A zero (or negative) duration is a cut: swap on the first update, never draw.
bool Fade::swap_ready() const {
    return duration <= 0.f || m_elapsed >= duration * 0.5f;
}

bool Fade::done() const {
    return m_elapsed >= duration;
}

float Fade::alpha() const {
    if (duration <= 0.f) return 0.f;

    const float half = duration * 0.5f;
    const float a = m_elapsed < half
                        ? m_elapsed / half             // fading out
                        : 1.f - (m_elapsed - half) / half;  // fading back in
    return std::clamp(a, 0.f, 1.f);
}

void Fade::draw(Renderer& renderer, int screen_w, int screen_h) {
    Color c = color;
    c.a *= alpha();
    if (c.a <= 0.f) return;

    // The overlay is a plain alpha-blended quad — reset whatever the last
    // drawable left set, since a transition draws after the whole scene.
    renderer.set_shader(nullptr);
    renderer.set_blend(BlendMode::Alpha);
    renderer.fill_rect(Rect(0.f, 0.f, static_cast<float>(screen_w),
                            static_cast<float>(screen_h)), c);
}

} // namespace loom
