#include <gtest/gtest.h>
#include "physics/physics.hpp"
#include <cmath>

using namespace loom;

// ── World creation ────────────────────────────────────────────────────────────

TEST(Physics, WorldCreation) {
    EXPECT_NO_THROW({ PhysicsWorld w; });
}

TEST(Physics, CustomGravity) {
    // Just verify it doesn't throw
    EXPECT_NO_THROW({ PhysicsWorld w(0.f, 500.f); });
}

// ── Body creation ─────────────────────────────────────────────────────────────

TEST(Physics, CreateStaticBody) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Static, {0.f, 0.f});
    ASSERT_NE(b, nullptr);
}

TEST(Physics, CreateDynamicBody) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Dynamic, {100.f, 200.f});
    ASSERT_NE(b, nullptr);

    Vec2 pos = b->position();
    EXPECT_NEAR(pos.x, 100.f, 0.5f);
    EXPECT_NEAR(pos.y, 200.f, 0.5f);
}

TEST(Physics, CreateKinematicBody) {
    PhysicsWorld w;
    EXPECT_NO_THROW(w.create_body(BodyType::Kinematic, {0.f, 0.f}));
}

// ── Shape attachment ──────────────────────────────────────────────────────────

TEST(Physics, AddBoxShape) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Dynamic, {0.f, 0.f});
    EXPECT_NO_THROW(b->add_box(16.f, 16.f));
}

TEST(Physics, AddCircleShape) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Dynamic, {0.f, 0.f});
    EXPECT_NO_THROW(b->add_circle(16.f));
}

// ── Simulation ────────────────────────────────────────────────────────────────

TEST(Physics, DynamicBodyFallsWithGravity) {
    PhysicsWorld world(0.f, 980.f); // 980 px/s² ≈ real gravity at PPM=64
    PhysicsBody* body = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    body->add_box(16.f, 16.f);

    Vec2 start = body->position();

    // Step 1 second (many small steps for accuracy)
    for (int i = 0; i < 60; ++i)
        world.step(1.f / 60.f);

    Vec2 end = body->position();
    // Body should have moved downward (positive Y = down in our convention)
    EXPECT_GT(end.y, start.y);
}

TEST(Physics, StaticBodyDoesNotMove) {
    PhysicsWorld world;
    PhysicsBody* floor = world.create_body(BodyType::Static, {0.f, 300.f});
    floor->add_box(400.f, 10.f);

    Vec2 before = floor->position();
    world.step(1.f / 60.f);
    Vec2 after  = floor->position();

    EXPECT_NEAR(before.x, after.x, 0.01f);
    EXPECT_NEAR(before.y, after.y, 0.01f);
}

TEST(Physics, ApplyImpulseChangesVelocity) {
    PhysicsWorld world(0.f, 0.f); // no gravity
    PhysicsBody* b = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    b->add_box(10.f, 10.f);

    Vec2 vel_before = b->linear_velocity();
    EXPECT_NEAR(vel_before.x, 0.f, 0.01f);

    b->apply_impulse({100.f, 0.f});
    world.step(1.f / 60.f);

    Vec2 vel_after = b->linear_velocity();
    EXPECT_GT(vel_after.x, 0.f); // moving right
}

TEST(Physics, SetPosition) {
    PhysicsWorld world;
    PhysicsBody* b = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    b->set_position({200.f, 150.f});

    Vec2 pos = b->position();
    EXPECT_NEAR(pos.x, 200.f, 1.f);
    EXPECT_NEAR(pos.y, 150.f, 1.f);
}

TEST(Physics, SetLinearVelocity) {
    PhysicsWorld world(0.f, 0.f);
    PhysicsBody* b = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    b->add_circle(8.f);
    b->set_linear_velocity({64.f, 0.f}); // 1 m/s at PPM=64

    world.step(1.f / 60.f);

    Vec2 pos = b->position();
    EXPECT_GT(pos.x, 0.f); // moved right
}

TEST(Physics, DestroyBody) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Dynamic, {0.f, 0.f});
    EXPECT_NO_THROW(w.destroy_body(b));
}

TEST(Physics, MultipleSteps) {
    PhysicsWorld w;
    EXPECT_NO_THROW({
        for (int i = 0; i < 120; ++i) w.step(1.f / 60.f);
    });
}

// ── Tags / user-data ────────────────────────────────────────────────────────────

TEST(Physics, BodyTagRoundTrips) {
    PhysicsWorld w;
    PhysicsBody* b = w.create_body(BodyType::Dynamic, {0.f, 0.f});
    EXPECT_EQ(b->tag(), "");
    b->set_tag("player");
    EXPECT_EQ(b->tag(), "player");
}

// ── Contact events ──────────────────────────────────────────────────────────────

TEST(Physics, ContactBeginFiresOnOverlap) {
    PhysicsWorld world(0.f, 0.f); // no gravity
    PhysicsBody* a = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    a->add_box(10.f, 10.f);
    a->set_tag("a");
    PhysicsBody* b = world.create_body(BodyType::Static, {15.f, 0.f});
    b->add_box(10.f, 10.f);
    b->set_tag("b");

    // Drive them into each other.
    a->set_linear_velocity({64.f, 0.f});

    bool saw_begin = false;
    for (int i = 0; i < 120 && !saw_begin; ++i) {
        world.step(1.f / 60.f);
        if (!world.contact_begins().empty()) saw_begin = true;
    }
    EXPECT_TRUE(saw_begin);
    // The pair should reference both of our bodies.
    const ContactPair& cp = world.contact_begins().front();
    EXPECT_TRUE((cp.body_a == a && cp.body_b == b) ||
                (cp.body_a == b && cp.body_b == a));
}

TEST(Physics, ContactCallbackInvoked) {
    PhysicsWorld world(0.f, 0.f);
    PhysicsBody* a = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    a->add_box(10.f, 10.f);
    PhysicsBody* b = world.create_body(BodyType::Static, {15.f, 0.f});
    b->add_box(10.f, 10.f);

    int hits = 0;
    world.on_contact_begin = [&](PhysicsBody*, PhysicsBody*){ ++hits; };
    a->set_linear_velocity({64.f, 0.f});

    for (int i = 0; i < 120 && hits == 0; ++i) world.step(1.f / 60.f);
    EXPECT_GT(hits, 0);
}

TEST(Physics, NonOverlappingBodiesNoContact) {
    PhysicsWorld world(0.f, 0.f);
    PhysicsBody* a = world.create_body(BodyType::Dynamic, {0.f, 0.f});
    a->add_box(10.f, 10.f);
    PhysicsBody* b = world.create_body(BodyType::Static, {500.f, 0.f});
    b->add_box(10.f, 10.f);

    for (int i = 0; i < 30; ++i) world.step(1.f / 60.f);
    EXPECT_TRUE(world.contact_begins().empty());
}

// ── Sensor events ───────────────────────────────────────────────────────────────

TEST(Physics, SensorReportsOverlapWithoutBlocking) {
    PhysicsWorld world(0.f, 0.f);
    // A static sensor at the origin.
    PhysicsBody* sensor = world.create_body(BodyType::Static, {0.f, 0.f});
    sensor->add_box(20.f, 20.f, 1.f, 0.3f, 0.f, /*is_sensor=*/true);
    sensor->set_tag("zone");
    // A dynamic body sliding straight through it.
    PhysicsBody* mover = world.create_body(BodyType::Dynamic, {-100.f, 0.f});
    mover->add_box(8.f, 8.f);
    mover->set_linear_velocity({200.f, 0.f});

    bool saw_sensor_begin = false;
    for (int i = 0; i < 120; ++i) {
        world.step(1.f / 60.f);
        if (!world.sensor_begins().empty()) saw_sensor_begin = true;
    }
    EXPECT_TRUE(saw_sensor_begin);
    // Sensor does not block: mover passes through to the far side.
    EXPECT_GT(mover->position().x, 50.f);
}

// ── Raycast ─────────────────────────────────────────────────────────────────────

TEST(Physics, RaycastHitsBody) {
    PhysicsWorld world(0.f, 0.f);
    PhysicsBody* wall = world.create_body(BodyType::Static, {100.f, 0.f});
    wall->add_box(10.f, 50.f);
    wall->set_tag("wall");
    world.step(1.f / 60.f); // let the shape register

    RaycastHit hit = world.raycast({0.f, 0.f}, {300.f, 0.f});
    EXPECT_TRUE(hit.hit);
    EXPECT_EQ(hit.body, wall);
    EXPECT_NEAR(hit.point.x, 90.f, 2.f); // near-side face of the wall
}

TEST(Physics, RaycastMisses) {
    PhysicsWorld world(0.f, 0.f);
    PhysicsBody* wall = world.create_body(BodyType::Static, {100.f, 500.f});
    wall->add_box(10.f, 10.f);
    world.step(1.f / 60.f);

    RaycastHit hit = world.raycast({0.f, 0.f}, {300.f, 0.f});
    EXPECT_FALSE(hit.hit);
    EXPECT_EQ(hit.body, nullptr);
}
