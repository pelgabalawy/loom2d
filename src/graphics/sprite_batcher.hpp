#pragma once
#include "sokol_gfx.h"
#include "math/vec2.hpp"
#include "math/rect.hpp"
#include "math/mat4.hpp"
#include "graphics/color.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/shader.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
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
// sharing a texture and draw state are merged, so a whole tilemap on one tileset
// is a single draw call. Draw order is preserved (painter's algorithm — correct
// for 2D alpha).
class SpriteBatcher {
public:
    SpriteBatcher() = default;
    ~SpriteBatcher();

    // Create GPU resources. Must be called once, after sg_setup().
    void init();

    // Set the view-projection applied to subsequently-submitted geometry. May be
    // changed mid-pass (e.g. world camera, then a fixed screen-space UI camera);
    // each change starts a new batch so one flush can mix both spaces.
    void set_view_projection(const Mat4& vp);

    // ── Draw state ──────────────────────────────────────────────────────────
    // Applied to every quad submitted from here on. Each drawable sets both at
    // the top of its draw(), so state never leaks from one drawable to the next.
    // Both are called once per drawable — including once per sprite in a 10k
    // sprite scene — so both take the fast path out when nothing has changed.
    void set_shader(const std::shared_ptr<Shader>& shader); // null = built-in
    void set_blend(BlendMode blend);

    void submit(const Texture& texture, const SpriteQuad& quad, const Color& tint);

    // ── Frame / pass lifecycle (driven by the Renderer) ──────────────────────
    // Once per frame, outside any pass: grow the vertex buffer if last frame
    // needed more than it holds, and reset the per-frame counters.
    void begin_frame();
    // Once per pass, just inside it. `offscreen` picks pipelines that match a
    // canvas (no depth attachment) and flips Y — see the note in the .cpp.
    void begin_pass(bool offscreen);
    // Upload and emit everything queued for the current pass.
    void flush();

    int draw_calls() const { return m_draw_calls; } // accumulated over the frame

private:
    struct Vertex { float x, y, u, v, r, g, b, a; };

    // start/count are in vertices. The shared_ptr keeps a shader alive until its
    // draw actually happens, even if the game drops its last reference mid-frame.
    struct Batch {
        sg_pipeline             pip;
        sg_view                 view;
        int                     start;
        int                     count;
        Mat4                    vp;
        unsigned                gen;
        std::shared_ptr<Shader> shader;
        uint32_t                shader_rev;
        std::vector<uint8_t>    uniforms; // snapshot taken when the batch opened
    };

    // Pipelines are cached per (shader, blend mode, render-target kind) and live
    // as long as the batcher. The lookup is a hash of all three, so it is done
    // only when the draw state actually changes — never per sprite.
    sg_pipeline pipeline_for(const Shader* shader, BlendMode blend, bool offscreen);
    sg_pipeline current_pipeline(); // resolves and caches on demand
    void        ensure_capacity(size_t vert_count);

    Mat4                    m_vp   = Mat4::identity();
    unsigned                m_gen  = 0;    // bumps on each set_view_projection
    std::shared_ptr<Shader> m_shader;      // current draw state
    BlendMode               m_blend = BlendMode::Alpha;
    bool                    m_offscreen = false;
    sg_pipeline             m_pip = {};    // pipeline for the state above
    bool                    m_pip_dirty = true;

    std::vector<Vertex> m_verts;
    std::vector<Batch>  m_batches;
    int                 m_draw_calls  = 0;
    size_t              m_frame_verts = 0; // appended so far this frame

    std::shared_ptr<Shader> m_default_shader;
    std::unordered_map<uint64_t, sg_pipeline> m_pipelines;

    sg_buffer  m_vbuf = {};
    sg_sampler m_smp  = {};
    size_t     m_capacity = 0; // vertices the GPU buffer can hold
    bool       m_ready = false;
};

} // namespace loom
