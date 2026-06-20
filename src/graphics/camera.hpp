#pragma once
#include "math/vec2.hpp"
#include "math/rect.hpp"

namespace loom {

class Camera {
public:
    Camera() = default;
    Camera(int viewport_w, int viewport_h);

    void set_viewport(int w, int h);

    Vec2  position() const { return m_position; }
    float zoom()     const { return m_zoom;     }
    float rotation() const { return m_rotation; }

    void set_position(Vec2 pos)   { m_position = pos;    }
    void set_zoom(float z)        { m_zoom = z > 0 ? z : 0.001f; }
    void set_rotation(float r)    { m_rotation = r;      }

    void move(Vec2 delta)         { m_position += delta;  }

    // Convert world-space → screen-space
    Vec2 world_to_screen(Vec2 world) const;

    // Convert screen-space → world-space
    Vec2 screen_to_world(Vec2 screen) const;

    // Axis-aligned world-space rect currently visible
    Rect visible_rect() const;

private:
    Vec2  m_position  = {0.f, 0.f};
    float m_zoom      = 1.f;
    float m_rotation  = 0.f;
    int   m_vp_w      = 800;
    int   m_vp_h      = 600;
};

} // namespace loom
