#include "physics/physics.hpp"
#include <algorithm>

namespace loom {

// ── PhysicsWorld ─────────────────────────────────────────────────────────────

PhysicsWorld::PhysicsWorld(float gravity_x, float gravity_y) {
    b2WorldDef def = b2DefaultWorldDef();
    // Convert pixel gravity to meters (PPM)
    def.gravity = {gravity_x * MPPM, gravity_y * MPPM};
    m_world = b2CreateWorld(&def);
}

PhysicsWorld::~PhysicsWorld() {
    m_bodies.clear(); // destroy bodies before world
    b2DestroyWorld(m_world);
}

// Map a shape back to its owning PhysicsBody (stored as shape user-data).
// Returns nullptr if the shape was destroyed before the event was drained.
static PhysicsBody* body_of(b2ShapeId shape) {
    if (!b2Shape_IsValid(shape)) return nullptr;
    return static_cast<PhysicsBody*>(b2Shape_GetUserData(shape));
}

void PhysicsWorld::step(float dt, int sub_steps) {
    b2World_Step(m_world, dt, sub_steps);

    m_contact_begins.clear();
    m_contact_ends.clear();
    m_sensor_begins.clear();
    m_sensor_ends.clear();

    b2ContactEvents ce = b2World_GetContactEvents(m_world);
    for (int i = 0; i < ce.beginCount; ++i) {
        PhysicsBody* a = body_of(ce.beginEvents[i].shapeIdA);
        PhysicsBody* b = body_of(ce.beginEvents[i].shapeIdB);
        m_contact_begins.push_back({a, b});
        if (on_contact_begin) on_contact_begin(a, b);
    }
    for (int i = 0; i < ce.endCount; ++i) {
        PhysicsBody* a = body_of(ce.endEvents[i].shapeIdA);
        PhysicsBody* b = body_of(ce.endEvents[i].shapeIdB);
        m_contact_ends.push_back({a, b});
        if (on_contact_end) on_contact_end(a, b);
    }

    b2SensorEvents se = b2World_GetSensorEvents(m_world);
    for (int i = 0; i < se.beginCount; ++i) {
        PhysicsBody* s = body_of(se.beginEvents[i].sensorShapeId);
        PhysicsBody* v = body_of(se.beginEvents[i].visitorShapeId);
        m_sensor_begins.push_back({s, v});
        if (on_sensor_begin) on_sensor_begin(s, v);
    }
    for (int i = 0; i < se.endCount; ++i) {
        PhysicsBody* s = body_of(se.endEvents[i].sensorShapeId);
        PhysicsBody* v = body_of(se.endEvents[i].visitorShapeId);
        m_sensor_ends.push_back({s, v});
        if (on_sensor_end) on_sensor_end(s, v);
    }
}

RaycastHit PhysicsWorld::raycast(Vec2 p1, Vec2 p2) const {
    b2Vec2 origin      = {p1.x * MPPM, p1.y * MPPM};
    b2Vec2 translation = {(p2.x - p1.x) * MPPM, (p2.y - p1.y) * MPPM};
    b2RayResult r = b2World_CastRayClosest(m_world, origin, translation,
                                           b2DefaultQueryFilter());
    RaycastHit out;
    out.hit = r.hit;
    if (r.hit) {
        out.point    = {r.point.x / MPPM, r.point.y / MPPM};
        out.normal   = {r.normal.x, r.normal.y}; // unit vector, no scaling
        out.fraction = r.fraction;
        out.body     = body_of(r.shapeId);
    }
    return out;
}

PhysicsBody* PhysicsWorld::create_body(BodyType type, Vec2 pixel_pos) {
    b2BodyDef def = b2DefaultBodyDef();
    def.position  = {pixel_pos.x * MPPM, pixel_pos.y * MPPM};
    switch (type) {
        case BodyType::Static:    def.type = b2_staticBody;    break;
        case BodyType::Kinematic: def.type = b2_kinematicBody; break;
        case BodyType::Dynamic:   def.type = b2_dynamicBody;   break;
    }
    b2BodyId id = b2CreateBody(m_world, &def);
    m_bodies.push_back(std::unique_ptr<PhysicsBody>(new PhysicsBody(id)));
    return m_bodies.back().get();
}

void PhysicsWorld::destroy_body(PhysicsBody* body) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [body](const auto& p){ return p.get() == body; });
    if (it != m_bodies.end()) {
        b2DestroyBody((*it)->m_body);
        m_bodies.erase(it);
    }
}

// ── PhysicsBody ──────────────────────────────────────────────────────────────

PhysicsBody::PhysicsBody(b2BodyId id) : m_body(id) {}

void PhysicsBody::add_box(float half_w, float half_h,
                           float density, float friction, float restitution,
                           bool is_sensor)
{
    b2Polygon box   = b2MakeBox(half_w * MPPM, half_h * MPPM);
    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density     = density;
    sdef.friction    = friction;
    sdef.restitution = restitution;
    sdef.isSensor    = is_sensor;
    sdef.userData    = this; // maps b2ShapeId → owning PhysicsBody in events
    b2CreatePolygonShape(m_body, &sdef, &box);
}

void PhysicsBody::add_circle(float radius,
                              float density, float friction, float restitution,
                              bool is_sensor)
{
    b2Circle circle  = {{0.f, 0.f}, radius * MPPM};
    b2ShapeDef sdef  = b2DefaultShapeDef();
    sdef.density     = density;
    sdef.friction    = friction;
    sdef.restitution = restitution;
    sdef.isSensor    = is_sensor;
    sdef.userData    = this; // maps b2ShapeId → owning PhysicsBody in events
    b2CreateCircleShape(m_body, &sdef, &circle);
}

Vec2 PhysicsBody::position() const {
    b2Vec2 p = b2Body_GetPosition(m_body);
    return {p.x / MPPM, p.y / MPPM};
}

float PhysicsBody::rotation() const {
    b2Rot r = b2Body_GetRotation(m_body);
    return b2Rot_GetAngle(r);
}

Vec2 PhysicsBody::linear_velocity() const {
    b2Vec2 v = b2Body_GetLinearVelocity(m_body);
    return {v.x / MPPM, v.y / MPPM};
}

void PhysicsBody::set_position(Vec2 pixel_pos) {
    b2Body_SetTransform(m_body,
        {pixel_pos.x * MPPM, pixel_pos.y * MPPM},
        b2Body_GetRotation(m_body));
}

void PhysicsBody::set_linear_velocity(Vec2 pixels_per_sec) {
    b2Body_SetLinearVelocity(m_body,
        {pixels_per_sec.x * MPPM, pixels_per_sec.y * MPPM});
}

void PhysicsBody::apply_impulse(Vec2 pixels_per_sec) {
    b2Vec2 imp = {pixels_per_sec.x * MPPM, pixels_per_sec.y * MPPM};
    b2Body_ApplyLinearImpulseToCenter(m_body, imp, true);
}

void PhysicsBody::apply_force(Vec2 pixel_newtons) {
    b2Vec2 f = {pixel_newtons.x * MPPM, pixel_newtons.y * MPPM};
    b2Body_ApplyForceToCenter(m_body, f, true);
}

} // namespace loom
