#pragma once
#include "math/vec2.hpp"
#include <box2d/box2d.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace loom {

// Pixels per meter — game code uses pixels; Box2D uses meters
constexpr float PPM  = 64.f;
constexpr float MPPM = 1.f / PPM;

enum class BodyType { Static, Kinematic, Dynamic };

class PhysicsBody;

// Begin/end contact between two solid shapes (order is arbitrary).
struct ContactPair {
    PhysicsBody* body_a = nullptr;
    PhysicsBody* body_b = nullptr;
};

// Begin/end overlap of a sensor by another (visitor) body.
struct SensorPair {
    PhysicsBody* sensor  = nullptr;
    PhysicsBody* visitor = nullptr;
};

// Result of PhysicsWorld::raycast (positions in pixels).
struct RaycastHit {
    bool         hit      = false;
    PhysicsBody* body     = nullptr;
    Vec2         point    {0.f, 0.f};
    Vec2         normal   {0.f, 0.f};
    float        fraction = 0.f;
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(float gravity_x = 0.f, float gravity_y = 980.f);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&)            = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Advance the simulation (call once per frame with dt in seconds).
    // Drains contact/sensor events and fires the on_* callbacks below.
    void step(float dt, int sub_steps = 4);

    // Create bodies (engine owns the body; returned ptr is non-owning)
    PhysicsBody* create_body(BodyType type, Vec2 pixel_position);
    void         destroy_body(PhysicsBody* body);

    // Nearest shape hit along the segment p1→p2 (pixels). hit==false if none.
    RaycastHit raycast(Vec2 p1, Vec2 p2) const;

    // Events from the most recent step() — valid until the next step().
    const std::vector<ContactPair>& contact_begins() const { return m_contact_begins; }
    const std::vector<ContactPair>& contact_ends()   const { return m_contact_ends;   }
    const std::vector<SensorPair>&  sensor_begins()  const { return m_sensor_begins;  }
    const std::vector<SensorPair>&  sensor_ends()    const { return m_sensor_ends;    }

    // Optional callbacks fired during step() as events are drained.
    std::function<void(PhysicsBody*, PhysicsBody*)> on_contact_begin;
    std::function<void(PhysicsBody*, PhysicsBody*)> on_contact_end;
    std::function<void(PhysicsBody*, PhysicsBody*)> on_sensor_begin; // (sensor, visitor)
    std::function<void(PhysicsBody*, PhysicsBody*)> on_sensor_end;   // (sensor, visitor)

    b2WorldId world_id() const { return m_world; }

private:
    b2WorldId                                 m_world;
    std::vector<std::unique_ptr<PhysicsBody>> m_bodies;

    std::vector<ContactPair> m_contact_begins;
    std::vector<ContactPair> m_contact_ends;
    std::vector<SensorPair>  m_sensor_begins;
    std::vector<SensorPair>  m_sensor_ends;
};

class PhysicsBody {
public:
    // Add a box collider (half-extents in pixels)
    void add_box(float half_w, float half_h,
                 float density = 1.f, float friction = 0.3f,
                 float restitution = 0.f, bool is_sensor = false);

    // Add a circle collider (radius in pixels)
    void add_circle(float radius,
                    float density = 1.f, float friction = 0.3f,
                    float restitution = 0.f, bool is_sensor = false);

    // User-assigned identifier, used to tell who-hit-what in contact events.
    const std::string& tag() const { return m_tag; }
    void set_tag(const std::string& t) { m_tag = t; }

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

    b2BodyId    m_body;
    std::string m_tag;
};

} // namespace loom
