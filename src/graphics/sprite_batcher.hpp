#pragma once
#include "sokol_gfx.h"
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include "math/mat4.hpp"
#include "graphics/color.hpp"
#include <vector>

namespace loom {

class Texture;

// Pure, GPU-free sprite geometry: the four world-space corners and their UVs.
// Corner order is TL, TR, BR, BL. Exposed as a free function so the transform
// math can be unit-tested without a GL context.
struct SpriteQuad {
    Vec2  pos[4];
    float uv[4][2];
};

// src is the sub-rectangle of the texture in pixels (w/h <= 0 means full texture).
SpriteQuad build_sprite_quad(Vec2 world_pos, float world_rot, Vec2 world_scale,
                             Vec2 origin, Rect src, int tex_w, int tex_h,
                             bool flip_x, bool flip_y);

// Batches textured quads into as few draw calls as possible: consecutive quads
// sharing a texture are merged, so a whole tilemap on one tileset is a single
// draw call. Draw order is preserved (painter's algorithm — correct for 2D alpha).
class SpriteBatcher {
public:
    SpriteBatcher() = default;
    ~SpriteBatcher();

    // Create GPU resources. Must be called once, after sg_setup().
    void init();

    void set_view_projection(const Mat4& vp) { m_vp = vp; }

    void submit(const Texture& texture, const SpriteQuad& quad, const Color& tint);

    // Upload and emit all queued geometry. Called once per frame by the Renderer.
    void flush();

    int draw_calls() const { return m_draw_calls; } // from the last flush

private:
    struct Vertex { float x, y, u, v, r, g, b, a; };
    struct Batch  { sg_view view; int start; int count; }; // start/count in vertices

    void ensure_capacity(size_t vert_count);

    Mat4                m_vp = Mat4::identity();
    std::vector<Vertex> m_verts;
    std::vector<Batch>  m_batches;
    int                 m_draw_calls = 0;

    sg_buffer   m_vbuf = {};
    sg_pipeline m_pip  = {};
    sg_shader   m_shd  = {};
    sg_sampler  m_smp  = {};
    size_t      m_capacity = 0; // vertices the GPU buffer can hold
    bool        m_ready = false;
};

} // namespace loom
