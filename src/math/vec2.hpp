#pragma once
#include <cmath>
#include <string>

namespace loom {

struct Vec2 {
    float x = 0.f, y = 0.f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2  operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2  operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2  operator*(float s)       const { return {x * s,   y * s};   }
    Vec2  operator/(float s)       const { return {x / s,   y / s};   }
    Vec2  operator-()              const { return {-x, -y};            }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)       { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s)       { x /= s;   y /= s;   return *this; }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    float length_sq() const { return x * x + y * y; }
    float length()    const { return std::sqrt(length_sq()); }

    Vec2 normalized() const {
        float l = length();
        return l > 0.f ? *this / l : Vec2{};
    }

    float dot(const Vec2& o)      const { return x * o.x + y * o.y; }
    float distance(const Vec2& o) const { return (*this - o).length(); }

    Vec2 lerp(const Vec2& o, float t) const {
        return {x + (o.x - x) * t, y + (o.y - y) * t};
    }

    Vec2 rotated(float radians) const {
        float c = std::cos(radians), s = std::sin(radians);
        return {x * c - y * s, x * s + y * c};
    }

    static Vec2 zero()  { return {0.f,  0.f};  }
    static Vec2 one()   { return {1.f,  1.f};  }
    static Vec2 up()    { return {0.f, -1.f};  }
    static Vec2 down()  { return {0.f,  1.f};  }
    static Vec2 left()  { return {-1.f, 0.f};  }
    static Vec2 right() { return {1.f,  0.f};  }

    std::string to_string() const {
        return "Vec2(" + std::to_string(x) + ", " + std::to_string(y) + ")";
    }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

} // namespace loom
