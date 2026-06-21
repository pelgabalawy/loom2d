#include "graphics/camera.hpp"
#include <cmath>

namespace loom {

Camera::Camera(int viewport_w, int viewport_h)
    : m_vp_w(viewport_w), m_vp_h(viewport_h) {}

void Camera::set_viewport(int w, int h) { m_vp_w = w; m_vp_h = h; }

Mat4 Camera::view_projection() const {
    // View: translate world relative to the camera, scale by zoom, rotate by
    // -rotation — exactly the chain world_to_screen() applies before centring.
    Mat4 view = Mat4::rotate_z(-m_rotation)
              * Mat4::scale(m_zoom, m_zoom)
              * Mat4::translate(-m_position.x, -m_position.y);

    // Projection: centred coordinates (y-down, like screen space) into clip.
    // Swapping bottom/top flips Y so screen-down maps to GL clip-up.
    float hw = m_vp_w * 0.5f;
    float hh = m_vp_h * 0.5f;
    Mat4 proj = Mat4::ortho(-hw, hw, hh, -hh);

    return proj * view;
}

Vec2 Camera::world_to_screen(Vec2 world) const {
    // Translate relative to camera, then zoom around screen center
    float cx = m_vp_w * 0.5f;
    float cy = m_vp_h * 0.5f;
    float rx = (world.x - m_position.x) * m_zoom;
    float ry = (world.y - m_position.y) * m_zoom;
    if (m_rotation != 0.f) {
        float c = std::cos(-m_rotation), s = std::sin(-m_rotation);
        float tx = rx * c - ry * s;
        float ty = rx * s + ry * c;
        rx = tx; ry = ty;
    }
    return {cx + rx, cy + ry};
}

Vec2 Camera::screen_to_world(Vec2 screen) const {
    float cx = m_vp_w * 0.5f;
    float cy = m_vp_h * 0.5f;
    float rx = (screen.x - cx) / m_zoom;
    float ry = (screen.y - cy) / m_zoom;
    if (m_rotation != 0.f) {
        float c = std::cos(m_rotation), s = std::sin(m_rotation);
        float tx = rx * c - ry * s;
        float ty = rx * s + ry * c;
        rx = tx; ry = ty;
    }
    return {rx + m_position.x, ry + m_position.y};
}

Rect Camera::visible_rect() const {
    float hw = (m_vp_w * 0.5f) / m_zoom;
    float hh = (m_vp_h * 0.5f) / m_zoom;
    return {m_position.x - hw, m_position.y - hh, hw * 2.f, hh * 2.f};
}

} // namespace loom
