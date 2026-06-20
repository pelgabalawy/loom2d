#include <gtest/gtest.h>
#include "scene/node.hpp"
#include "scene/scene.hpp"
#include <memory>
#include <cmath>

using namespace loom;

// ── Node transform ────────────────────────────────────────────────────────────

TEST(Node, DefaultTransform) {
    Node n;
    EXPECT_EQ(n.position(), Vec2(0.f, 0.f));
    EXPECT_FLOAT_EQ(n.rotation(), 0.f);
    EXPECT_EQ(n.scale(), Vec2(1.f, 1.f));
    EXPECT_EQ(n.world_position(), Vec2(0.f, 0.f));
}

TEST(Node, SetPosition) {
    auto n = std::make_shared<Node>();
    n->set_position({100.f, 200.f});
    EXPECT_EQ(n->position(), Vec2(100.f, 200.f));
    EXPECT_EQ(n->world_position(), Vec2(100.f, 200.f));
}

TEST(Node, SetXY) {
    Node n;
    n.set_x(42.f); n.set_y(99.f);
    EXPECT_FLOAT_EQ(n.x(), 42.f);
    EXPECT_FLOAT_EQ(n.y(), 99.f);
}

TEST(Node, SetRotation) {
    Node n;
    n.set_rotation(1.5f);
    EXPECT_FLOAT_EQ(n.rotation(), 1.5f);
}

TEST(Node, SetScale) {
    Node n;
    n.set_scale(Vec2{2.f, 3.f});
    EXPECT_EQ(n.scale(), Vec2(2.f, 3.f));
}

TEST(Node, UniformScale) {
    Node n;
    n.set_scale(5.f);
    EXPECT_EQ(n.scale(), Vec2(5.f, 5.f));
}

// ── Parent-child hierarchy ────────────────────────────────────────────────────

TEST(Node, ChildWorldPosition_NoRotation) {
    auto parent = std::make_shared<Node>("parent");
    auto child  = std::make_shared<Node>("child");

    parent->set_position({100.f, 0.f});
    child->set_position({50.f, 0.f});
    parent->add_child(child);

    Vec2 wp = child->world_position();
    EXPECT_NEAR(wp.x, 150.f, 1e-4f);
    EXPECT_NEAR(wp.y,   0.f, 1e-4f);
}

TEST(Node, ChildWorldPosition_WithParentScale) {
    auto parent = std::make_shared<Node>();
    auto child  = std::make_shared<Node>();

    parent->set_position({0.f, 0.f});
    parent->set_scale(2.f);
    child->set_position({10.f, 0.f});
    parent->add_child(child);

    Vec2 wp = child->world_position();
    EXPECT_NEAR(wp.x, 20.f, 1e-4f); // 10 * scale(2)
}

TEST(Node, ChildWorldRotation_Accumulated) {
    auto parent = std::make_shared<Node>();
    auto child  = std::make_shared<Node>();

    parent->set_rotation(1.f);
    child->set_rotation(0.5f);
    parent->add_child(child);

    EXPECT_NEAR(child->world_rotation(), 1.5f, 1e-6f);
}

TEST(Node, ParentChildLink) {
    auto parent = std::make_shared<Node>("p");
    auto child  = std::make_shared<Node>("c");
    parent->add_child(child);

    EXPECT_EQ(parent->children().size(), 1u);
    EXPECT_EQ(child->parent(), parent.get());
}

TEST(Node, RemoveChild) {
    auto parent = std::make_shared<Node>();
    auto child  = std::make_shared<Node>();
    parent->add_child(child);
    EXPECT_EQ(parent->children().size(), 1u);

    parent->remove_child(child.get());
    EXPECT_EQ(parent->children().size(), 0u);
    EXPECT_EQ(child->parent(), nullptr);
}

TEST(Node, RemoveFromParent) {
    auto parent = std::make_shared<Node>();
    auto child  = std::make_shared<Node>();
    parent->add_child(child);
    child->remove_from_parent();
    EXPECT_EQ(parent->children().size(), 0u);
    EXPECT_EQ(child->parent(), nullptr);
}

TEST(Node, MultipleChildren) {
    auto parent = std::make_shared<Node>();
    for (int i = 0; i < 5; ++i)
        parent->add_child(std::make_shared<Node>());
    EXPECT_EQ(parent->children().size(), 5u);
}

TEST(Node, ReparentMovesChild) {
    auto p1 = std::make_shared<Node>();
    auto p2 = std::make_shared<Node>();
    auto child = std::make_shared<Node>();

    p1->add_child(child);
    EXPECT_EQ(p1->children().size(), 1u);

    // Adding to p2 should remove from p1
    p2->add_child(child);
    EXPECT_EQ(p1->children().size(), 0u);
    EXPECT_EQ(p2->children().size(), 1u);
    EXPECT_EQ(child->parent(), p2.get());
}

TEST(Node, VisibilityDefault) {
    Node n;
    EXPECT_TRUE(n.visible);
}

TEST(Node, NameDefault) {
    Node n;
    EXPECT_TRUE(n.name.empty());

    Node named("hero");
    EXPECT_EQ(named.name, "hero");
}

TEST(Node, DeepHierarchyWorldPosition) {
    auto root  = std::make_shared<Node>();
    auto mid   = std::make_shared<Node>();
    auto leaf  = std::make_shared<Node>();

    root->set_position({10.f, 0.f});
    mid->set_position({10.f, 0.f});
    leaf->set_position({10.f, 0.f});

    root->add_child(mid);
    mid->add_child(leaf);

    Vec2 wp = leaf->world_position();
    EXPECT_NEAR(wp.x, 30.f, 1e-4f);
}

// ── Scene ─────────────────────────────────────────────────────────────────────

TEST(Scene, AddAndClear) {
    Scene scene;
    scene.add(std::make_shared<Node>());
    scene.add(std::make_shared<Node>());
    EXPECT_EQ(scene.root().children().size(), 2u);

    scene.clear();
    EXPECT_EQ(scene.root().children().size(), 0u);
}

TEST(Scene, UpdatePropagates) {
    // Just verify it doesn't crash
    Scene scene;
    auto node = std::make_shared<Node>();
    scene.add(node);
    EXPECT_NO_THROW(scene.update(0.016f));
}
