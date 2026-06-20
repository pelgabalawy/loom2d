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

void PhysicsWorld::step(float dt, int sub_steps) {
    b2World_Step(m_world, dt, sub_steps);
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
                           float density, float friction, float restitution)
{
    b2Polygon box   = b2MakeBox(half_w * MPPM, half_h * MPPM);
    b2ShapeDef sdef = b2DefaultShapeDef();
    sdef.density     = density;
    sdef.friction    = friction;
    sdef.restitution = restitution;
    b2CreatePolygonShape(m_body, &sdef, &box);
}

void PhysicsBody::add_circle(float radius,
                              float density, float friction, float restitution)
{
    b2Circle circle  = {{0.f, 0.f}, radius * MPPM};
    b2ShapeDef sdef  = b2DefaultShapeDef();
    sdef.density     = density;
    sdef.friction    = friction;
    sdef.restitution = restitution;
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
