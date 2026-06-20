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
