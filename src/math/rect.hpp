#pragma once
#include "math/vec2.hpp"
#include <algorithm>

namespace loom {

struct Rect {
    float x = 0.f, y = 0.f, w = 0.f, h = 0.f;

    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
    Rect(Vec2 pos, Vec2 size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    float left()   const { return x; }
    float right()  const { return x + w; }
    float top()    const { return y; }
    float bottom() const { return y + h; }

    Vec2 position() const { return {x, y}; }
    Vec2 size()     const { return {w, h}; }
    Vec2 center()   const { return {x + w * 0.5f, y + h * 0.5f}; }

    bool contains(const Vec2& p) const {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }

    bool contains(const Rect& o) const {
        return o.x >= x && o.right() <= right() &&
               o.y >= y && o.bottom() <= bottom();
    }

    bool intersects(const Rect& o) const {
        return x < o.right() && right() > o.x &&
               y < o.bottom() && bottom() > o.y;
    }

    Rect intersection(const Rect& o) const {
        float lx = std::max(x, o.x);
        float ly = std::max(y, o.y);
        float rx = std::min(right(),  o.right());
        float ry = std::min(bottom(), o.bottom());
        if (rx < lx || ry < ly) return {};
        return {lx, ly, rx - lx, ry - ly};
    }

    Rect expanded(float amount) const {
        return {x - amount, y - amount, w + amount * 2.f, h + amount * 2.f};
    }

    bool operator==(const Rect& o) const {
        return x == o.x && y == o.y && w == o.w && h == o.h;
    }
};

} // namespace loom
