#pragma once
#include <array>
#include <cmath>

namespace loom {

// Minimal 4x4 matrix, column-major (the layout OpenGL / sokol_gfx expect).
// Element at (row, col) is m[col * 4 + row]. Just enough for 2D ortho cameras.
struct Mat4 {
    std::array<float, 16> m{};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.f;
        return r;
    }

    static Mat4 translate(float x, float y, float z = 0.f) {
        Mat4 r = identity();
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }

    static Mat4 scale(float x, float y, float z = 1.f) {
        Mat4 r = identity();
        r.m[0] = x; r.m[5] = y; r.m[10] = z;
        return r;
    }

    // Rotation about the Z axis (2D), angle in radians.
    static Mat4 rotate_z(float radians) {
        Mat4 r = identity();
        float c = std::cos(radians), s = std::sin(radians);
        r.m[0] = c;  r.m[1] = s;
        r.m[4] = -s; r.m[5] = c;
        return r;
    }

    // Orthographic projection mapping [l,r]x[b,t]x[n,f] into clip [-1,1].
    static Mat4 ortho(float l, float r, float b, float t,
                      float n = -1.f, float f = 1.f) {
        Mat4 o;
        o.m[0]  = 2.f / (r - l);
        o.m[5]  = 2.f / (t - b);
        o.m[10] = -2.f / (f - n);
        o.m[12] = -(r + l) / (r - l);
        o.m[13] = -(t + b) / (t - b);
        o.m[14] = -(f + n) / (f - n);
        o.m[15] = 1.f;
        return o;
    }

    // Matrix product (this * rhs), column-major.
    Mat4 operator*(const Mat4& rhs) const {
        Mat4 out;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row) {
                float sum = 0.f;
                for (int k = 0; k < 4; ++k)
                    sum += m[k * 4 + row] * rhs.m[col * 4 + k];
                out.m[col * 4 + row] = sum;
            }
        return out;
    }

    // Transform a homogeneous point; returns {x, y, z, w} in clip space.
    std::array<float, 4> transform(float x, float y, float z = 0.f,
                                   float w = 1.f) const {
        return {
            m[0] * x + m[4] * y + m[8]  * z + m[12] * w,
            m[1] * x + m[5] * y + m[9]  * z + m[13] * w,
            m[2] * x + m[6] * y + m[10] * z + m[14] * w,
            m[3] * x + m[7] * y + m[11] * z + m[15] * w,
        };
    }
};

} // namespace loom
