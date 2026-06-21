#pragma once

namespace loom {

struct Color {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

    Color() = default;
    Color(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a) {}

    static Color black()      { return {0.f,   0.f,   0.f,   1.f}; }
    static Color white()      { return {1.f,   1.f,   1.f,   1.f}; }
    static Color red()        { return {1.f,   0.f,   0.f,   1.f}; }
    static Color green()      { return {0.f,   1.f,   0.f,   1.f}; }
    static Color blue()       { return {0.f,   0.f,   1.f,   1.f}; }
    static Color yellow()     { return {1.f,   1.f,   0.f,   1.f}; }
    static Color transparent(){ return {0.f,   0.f,   0.f,   0.f}; }
    static Color cornflower() { return {0.39f, 0.58f, 0.93f, 1.f}; }
};

} // namespace loom
