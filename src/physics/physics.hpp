#pragma once
#include "math/vec2.hpp"
#include <box2d/box2d.h>
#include <memory>
#include <vector>

namespace loom {

// Pixels per meter — game code uses pixels; Box2D uses meters
constexpr float PPM  = 64.f;
constexpr float MPPM = 1.f / PPM;

enum class BodyType { Static, Kinematic, Dynamic };

class PhysicsBody;

class PhysicsWorld {
public:
    explicit PhysicsWorld(float gravity_x = 0.f, float gravity_y = 980.f);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&)            = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Advance the simulation (call once per frame with dt in seconds)
    void step(float dt, int sub_steps = 4);

    // Create bodies (engine owns the body; returned ptr is non-owning)
    PhysicsBody* create_body(BodyType type, Vec2 pixel_position);
    void         destroy_body(PhysicsBody* body);

    b2WorldId world_id() const { return m_world; }

private:
    b2WorldId                           m_world;
    std::vector<std::unique_ptr<PhysicsBody>> m_bodies;
};

class PhysicsBody {
public:
    // Add a box collider (half-extents in pixels)
    void add_box(float half_w, float half_h,
                 float density = 1.f, float friction = 0.3f,
                 float restitution = 0.f);

    // Add a circle collider (radius in pixels)
    void add_circle(float radius,
                    float density = 1.f, float friction = 0.3f,
                    float restitution = 0.f);

    Vec2  position()        const; // pixels
    float rotation()        const; // radians
    Vec2  linear_velocity() const; // pixels/sec

    void set_position(Vec2 pixel_pos);
    void set_linear_velocity(Vec2 pixels_per_sec);
    void apply_impulse(Vec2 pixels_per_sec);
    void apply_force(Vec2 pixel_newtons);

    b2BodyId body_id() const { return m_body; }

private:
    friend class PhysicsWorld;
    explicit PhysicsBody(b2BodyId id);

    b2BodyId m_body;
};

} // namespace loom
