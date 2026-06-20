#include <gtest/gtest.h>
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include "graphics/renderer.hpp" // Color
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

using namespace loom;

// ── Vec2 ─────────────────────────────────────────────────────────────────────

TEST(Vec2, DefaultIsZero) {
    Vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
}

TEST(Vec2, Construction) {
    Vec2 v{3.f, 4.f};
    EXPECT_FLOAT_EQ(v.x, 3.f);
    EXPECT_FLOAT_EQ(v.y, 4.f);
}

TEST(Vec2, Addition) {
    Vec2 a{1.f, 2.f}, b{3.f, 4.f};
    Vec2 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 4.f);
    EXPECT_FLOAT_EQ(c.y, 6.f);
}

TEST(Vec2, Subtraction) {
    Vec2 a{5.f, 3.f}, b{2.f, 1.f};
    Vec2 c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3.f);
    EXPECT_FLOAT_EQ(c.y, 2.f);
}

TEST(Vec2, ScalarMultiply) {
    Vec2 v{2.f, 3.f};
    Vec2 r = v * 2.f;
    EXPECT_FLOAT_EQ(r.x, 4.f);
    EXPECT_FLOAT_EQ(r.y, 6.f);
}

TEST(Vec2, ScalarDivide) {
    Vec2 v{6.f, 4.f};
    Vec2 r = v / 2.f;
    EXPECT_FLOAT_EQ(r.x, 3.f);
    EXPECT_FLOAT_EQ(r.y, 2.f);
}

TEST(Vec2, Negation) {
    Vec2 v{1.f, -2.f};
    Vec2 r = -v;
    EXPECT_FLOAT_EQ(r.x, -1.f);
    EXPECT_FLOAT_EQ(r.y,  2.f);
}

TEST(Vec2, Length) {
    Vec2 v{3.f, 4.f};
    EXPECT_FLOAT_EQ(v.length(), 5.f);
}

TEST(Vec2, LengthSq) {
    Vec2 v{3.f, 4.f};
    EXPECT_FLOAT_EQ(v.length_sq(), 25.f);
}

TEST(Vec2, Normalized) {
    Vec2 v{3.f, 4.f};
    Vec2 n = v.normalized();
    EXPECT_NEAR(n.length(), 1.f, 1e-6f);
    EXPECT_NEAR(n.x, 0.6f, 1e-6f);
    EXPECT_NEAR(n.y, 0.8f, 1e-6f);
}

TEST(Vec2, NormalizeZeroReturnsZero) {
    Vec2 zero;
    Vec2 n = zero.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.f);
    EXPECT_FLOAT_EQ(n.y, 0.f);
}

TEST(Vec2, DotProduct) {
    Vec2 a{1.f, 0.f}, b{0.f, 1.f};
    EXPECT_FLOAT_EQ(a.dot(b), 0.f);   // perpendicular

    Vec2 c{1.f, 0.f};
    EXPECT_FLOAT_EQ(c.dot(c), 1.f);   // self = 1 (unit vec)
}

TEST(Vec2, Distance) {
    Vec2 a{0.f, 0.f}, b{3.f, 4.f};
    EXPECT_FLOAT_EQ(a.distance(b), 5.f);
}

TEST(Vec2, Lerp) {
    Vec2 a{0.f, 0.f}, b{10.f, 10.f};
    Vec2 m = a.lerp(b, 0.5f);
    EXPECT_FLOAT_EQ(m.x, 5.f);
    EXPECT_FLOAT_EQ(m.y, 5.f);

    Vec2 start = a.lerp(b, 0.f);
    EXPECT_FLOAT_EQ(start.x, 0.f);

    Vec2 end = a.lerp(b, 1.f);
    EXPECT_FLOAT_EQ(end.x, 10.f);
}

TEST(Vec2, Rotated90Degrees) {
    Vec2 v{1.f, 0.f};
    Vec2 r = v.rotated(static_cast<float>(M_PI / 2.0));
    EXPECT_NEAR(r.x,  0.f, 1e-6f);
    EXPECT_NEAR(r.y,  1.f, 1e-6f);
}

TEST(Vec2, Equality) {
    Vec2 a{1.f, 2.f}, b{1.f, 2.f}, c{1.f, 3.f};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(Vec2, StaticHelpers) {
    EXPECT_EQ(Vec2::zero(),  Vec2(0.f,  0.f));
    EXPECT_EQ(Vec2::one(),   Vec2(1.f,  1.f));
    EXPECT_EQ(Vec2::up(),    Vec2(0.f, -1.f));
    EXPECT_EQ(Vec2::down(),  Vec2(0.f,  1.f));
    EXPECT_EQ(Vec2::left(),  Vec2(-1.f, 0.f));
    EXPECT_EQ(Vec2::right(), Vec2(1.f,  0.f));
}

TEST(Vec2, CompoundAssignment) {
    Vec2 v{1.f, 2.f};
    v += Vec2{1.f, 1.f};
    EXPECT_EQ(v, Vec2(2.f, 3.f));
    v -= Vec2{1.f, 1.f};
    EXPECT_EQ(v, Vec2(1.f, 2.f));
    v *= 2.f;
    EXPECT_EQ(v, Vec2(2.f, 4.f));
    v /= 2.f;
    EXPECT_EQ(v, Vec2(1.f, 2.f));
}

// ── Rect ─────────────────────────────────────────────────────────────────────

TEST(Rect, DefaultIsZero) {
    Rect r;
    EXPECT_FLOAT_EQ(r.x, 0.f); EXPECT_FLOAT_EQ(r.y, 0.f);
    EXPECT_FLOAT_EQ(r.w, 0.f); EXPECT_FLOAT_EQ(r.h, 0.f);
}

TEST(Rect, Edges) {
    Rect r{10.f, 20.f, 100.f, 50.f};
    EXPECT_FLOAT_EQ(r.left(),   10.f);
    EXPECT_FLOAT_EQ(r.right(),  110.f);
    EXPECT_FLOAT_EQ(r.top(),    20.f);
    EXPECT_FLOAT_EQ(r.bottom(), 70.f);
}

TEST(Rect, Center) {
    Rect r{0.f, 0.f, 100.f, 100.f};
    Vec2 c = r.center();
    EXPECT_FLOAT_EQ(c.x, 50.f);
    EXPECT_FLOAT_EQ(c.y, 50.f);
}

TEST(Rect, ContainsPoint) {
    Rect r{0.f, 0.f, 100.f, 100.f};
    EXPECT_TRUE(r.contains(Vec2{50.f, 50.f}));
    EXPECT_TRUE(r.contains(Vec2{0.f,  0.f}));  // border included
    EXPECT_FALSE(r.contains(Vec2{101.f, 50.f}));
    EXPECT_FALSE(r.contains(Vec2{-1.f,  50.f}));
}

TEST(Rect, Intersects) {
    Rect a{0.f, 0.f, 100.f, 100.f};
    Rect b{50.f, 50.f, 100.f, 100.f};
    Rect c{200.f, 200.f, 10.f, 10.f};
    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
}

TEST(Rect, Intersection) {
    Rect a{0.f, 0.f, 100.f, 100.f};
    Rect b{50.f, 50.f, 100.f, 100.f};
    Rect i = a.intersection(b);
    EXPECT_FLOAT_EQ(i.x, 50.f);
    EXPECT_FLOAT_EQ(i.y, 50.f);
    EXPECT_FLOAT_EQ(i.w, 50.f);
    EXPECT_FLOAT_EQ(i.h, 50.f);
}

TEST(Rect, NoIntersectionIsEmpty) {
    Rect a{0.f, 0.f, 10.f, 10.f};
    Rect b{20.f, 20.f, 10.f, 10.f};
    Rect i = a.intersection(b);
    EXPECT_FLOAT_EQ(i.w, 0.f);
    EXPECT_FLOAT_EQ(i.h, 0.f);
}

TEST(Rect, Expanded) {
    Rect r{10.f, 10.f, 50.f, 50.f};
    Rect e = r.expanded(5.f);
    EXPECT_FLOAT_EQ(e.x, 5.f);
    EXPECT_FLOAT_EQ(e.y, 5.f);
    EXPECT_FLOAT_EQ(e.w, 60.f);
    EXPECT_FLOAT_EQ(e.h, 60.f);
}

// ── Color ─────────────────────────────────────────────────────────────────────

TEST(Color, DefaultConstruction) {
    Color c;
    EXPECT_FLOAT_EQ(c.r, 0.f); EXPECT_FLOAT_EQ(c.g, 0.f);
    EXPECT_FLOAT_EQ(c.b, 0.f); EXPECT_FLOAT_EQ(c.a, 1.f);
}

TEST(Color, ExplicitConstruction) {
    Color c{0.5f, 0.25f, 0.75f, 0.9f};
    EXPECT_FLOAT_EQ(c.r, 0.5f);
    EXPECT_FLOAT_EQ(c.g, 0.25f);
    EXPECT_FLOAT_EQ(c.b, 0.75f);
    EXPECT_FLOAT_EQ(c.a, 0.9f);
}

TEST(Color, Presets) {
    EXPECT_EQ(Color::white().r, 1.f);
    EXPECT_EQ(Color::black().r, 0.f);
    EXPECT_EQ(Color::red().r,   1.f);  EXPECT_EQ(Color::red().g, 0.f);
    EXPECT_EQ(Color::green().g, 1.f);
    EXPECT_EQ(Color::blue().b,  1.f);
    EXPECT_EQ(Color::transparent().a, 0.f);
}
