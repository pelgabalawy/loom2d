#include "graphics/sprite_batcher.hpp"
#include "graphics/texture.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace loom {

// ── Pure geometry (unit-tested) ─────────────────────────────────────────────

SpriteQuad build_sprite_quad(Vec2 wpos, float wrot, Vec2 wscale, Vec2 origin,
                             Rect src, int tex_w, int tex_h,
                             bool flip_x, bool flip_y) {
    float sx = src.x, sy = src.y, sw = src.w, sh = src.h;
    if (sw <= 0.f || sh <= 0.f) {
        sx = 0.f; sy = 0.f;
        sw = static_cast<float>(tex_w);
        sh = static_cast<float>(tex_h);
    }

    // World-space size (camera zoom is applied later by the view-projection).
    float w = sw * std::abs(wscale.x);
    float h = sh * std::abs(wscale.y);

    // Corners relative to the pivot (origin normalized 0..1), screen y-down.
    float l = -origin.x * w, r = (1.f - origin.x) * w;
    float t = -origin.y * h, b = (1.f - origin.y) * h;
    Vec2 local[4] = { {l, t}, {r, t}, {r, b}, {l, b} }; // TL, TR, BR, BL

    SpriteQuad q;
    float c = std::cos(wrot), s = std::sin(wrot);
    for (int i = 0; i < 4; ++i) {
        float x = local[i].x, y = local[i].y;
        q.pos[i] = { wpos.x + (x * c - y * s), wpos.y + (x * s + y * c) };
    }

    float u0 = sx / tex_w, u1 = (sx + sw) / tex_w;
    float v0 = sy / tex_h, v1 = (sy + sh) / tex_h;
    if (flip_x) std::swap(u0, u1);
    if (flip_y) std::swap(v0, v1);
    q.uv[0][0] = u0; q.uv[0][1] = v0;
    q.uv[1][0] = u1; q.uv[1][1] = v0;
    q.uv[2][0] = u1; q.uv[2][1] = v1;
    q.uv[3][0] = u0; q.uv[3][1] = v1;
    return q;
}

// The shader every drawable gets unless it asks for its own: sample the texture,
// multiply by the tint. Written against the same effect() interface the game's
// shaders use, so there is exactly one place that defines what a loom2d fragment
// shader looks like.
static const char* DEFAULT_EFFECT =
    "vec4 effect(vec4 color, sampler2D tex, vec2 uv) {\n"
    "    return texture(tex, uv) * color;\n"
    "}\n";

// A first frame that needs more than this grows the buffer for the next one; big
// enough that no realistic scene (this holds ~10k sprites) ever pays that cost.
static constexpr size_t INITIAL_VERTS = 65536;

// ── SpriteBatcher ───────────────────────────────────────────────────────────

SpriteBatcher::~SpriteBatcher() {
    // sg_shutdown() (called by ~Renderer) frees everything; only clean up
    // explicitly if sokol is still alive when this batcher dies.
    if (m_ready && sg_isvalid()) {
        for (const auto& entry : m_pipelines) sg_destroy_pipeline(entry.second);
        sg_destroy_sampler(m_smp);
        if (m_vbuf.id != SG_INVALID_ID) sg_destroy_buffer(m_vbuf);
    }
}

void SpriteBatcher::init() {
    m_default_shader = Shader::from_source(DEFAULT_EFFECT);

    sg_sampler_desc smp = {};
    smp.min_filter = SG_FILTER_NEAREST;
    smp.mag_filter = SG_FILTER_NEAREST;
    smp.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    smp.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    m_smp = sg_make_sampler(&smp);

    m_ready = true;
    ensure_capacity(INITIAL_VERTS);
}

sg_pipeline SpriteBatcher::pipeline_for(const Shader* shader, BlendMode blend,
                                        bool offscreen) {
    const Shader* shd = shader ? shader : m_default_shader.get();
    const uint64_t key = (static_cast<uint64_t>(shd->handle().id) << 32)
                       | (static_cast<uint64_t>(blend) << 1)
                       | (offscreen ? 1u : 0u);

    auto it = m_pipelines.find(key);
    if (it != m_pipelines.end()) return it->second;

    sg_pipeline_desc pd = {};
    pd.shader = shd->handle();
    pd.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2; // a_pos
    pd.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // a_uv
    pd.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4; // a_color
    pd.primitive_type  = SG_PRIMITIVETYPE_TRIANGLES;
    pd.colors[0].blend = blend_state_for(blend);
    if (offscreen) {
        // A canvas is colour-only, and sokol requires the pipeline's attachment
        // formats to match the pass it runs in — hence a separate pipeline for
        // the same shader when it draws into a canvas rather than the window.
        pd.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pd.depth.pixel_format     = SG_PIXELFORMAT_NONE;
        pd.color_count            = 1;
    }

    sg_pipeline pip = sg_make_pipeline(&pd);
    m_pipelines.emplace(key, pip);
    return pip;
}

void SpriteBatcher::ensure_capacity(size_t need) {
    if (m_vbuf.id != SG_INVALID_ID && need <= m_capacity) return;
    if (m_vbuf.id != SG_INVALID_ID) sg_destroy_buffer(m_vbuf);

    size_t cap = m_capacity ? m_capacity : INITIAL_VERTS;
    while (cap < need) cap *= 2;

    sg_buffer_desc bd = {};
    bd.size = cap * sizeof(Vertex);
    bd.usage.vertex_buffer  = true;
    bd.usage.dynamic_update = true;
    m_vbuf = sg_make_buffer(&bd);
    m_capacity = cap;
}

void SpriteBatcher::begin_frame() {
    // Resizing the buffer is only safe between frames, so a frame that needed
    // more room than it had (its overflowing draws were dropped by sokol) grows
    // the buffer here, and the next frame renders in full.
    if (m_frame_verts > m_capacity) ensure_capacity(m_frame_verts);
    m_frame_verts = 0;
    m_draw_calls  = 0;
}

void SpriteBatcher::begin_pass(bool offscreen) {
    m_offscreen = offscreen;
    m_shader.reset();
    m_blend     = BlendMode::Alpha;
    m_pip_dirty = true;
}

void SpriteBatcher::set_view_projection(const Mat4& vp) {
    m_vp = vp;
    ++m_gen; // force a new batch so geometry isn't merged across a vp change
}

void SpriteBatcher::set_shader(const std::shared_ptr<Shader>& shader) {
    // Every sprite in the scene calls this every frame, nearly always with the
    // value already set. Comparing the raw pointers first keeps the common case
    // free of shared_ptr refcount traffic (atomics, once per sprite per frame).
    if (shader.get() == m_shader.get()) return;
    m_shader    = shader;
    m_pip_dirty = true;
}

void SpriteBatcher::set_blend(BlendMode blend) {
    if (blend == m_blend) return;
    m_blend     = blend;
    m_pip_dirty = true;
}

sg_pipeline SpriteBatcher::current_pipeline() {
    if (m_pip_dirty) {
        m_pip       = pipeline_for(m_shader.get(), m_blend, m_offscreen);
        m_pip_dirty = false;
    }
    return m_pip;
}

void SpriteBatcher::submit(const Texture& texture, const SpriteQuad& q,
                           const Color& tint) {
    const sg_view     view = texture.view();
    const sg_pipeline pip  = current_pipeline();
    const uint32_t    rev  = m_shader ? m_shader->revision() : 0;
    static const int  idx[6] = {0, 1, 2, 0, 2, 3}; // two triangles of TL,TR,BR,BL

    bool merged = false;
    if (!m_batches.empty()) {
        Batch& b = m_batches.back();
        // A change of uniform values (a new revision) has to break the batch:
        // the values are baked in per draw call, so quads drawn with different
        // ones can't share it.
        merged = b.pip.id == pip.id && b.view.id == view.id && b.gen == m_gen
              && b.shader == m_shader && b.shader_rev == rev;
        if (merged) b.count += 6;
    }
    if (!merged) {
        // Y-flip for canvases: OpenGL writes the top of clip space to the LAST
        // row of a render target, so a canvas drawn with the normal projection
        // comes out upside-down when it is later sampled as a texture (where we
        // treat v=0 as the top row). Negating clip-space Y for offscreen passes
        // stores the image the right way up, which means a canvas texture then
        // behaves exactly like a loaded one everywhere else.
        const Mat4 vp = m_offscreen ? Mat4::scale(1.f, -1.f, 1.f) * m_vp : m_vp;

        Batch b;
        b.pip        = pip;
        b.view       = view;
        b.start      = static_cast<int>(m_verts.size());
        b.count      = 6;
        b.vp         = vp;
        b.gen        = m_gen;
        b.shader     = m_shader;
        b.shader_rev = rev;
        if (m_shader) b.uniforms = m_shader->uniform_data();
        m_batches.push_back(std::move(b));
    }

    for (int i = 0; i < 6; ++i) {
        const int k = idx[i];
        m_verts.push_back({ q.pos[k].x, q.pos[k].y, q.uv[k][0], q.uv[k][1],
                            tint.r, tint.g, tint.b, tint.a });
    }
}

void SpriteBatcher::flush() {
    if (m_verts.empty()) return;

    // Append rather than update: a frame that renders into one or more canvases
    // before the window flushes the batcher once per pass, and sokol allows only
    // a single sg_update_buffer per buffer per frame.
    const sg_range data{ m_verts.data(), m_verts.size() * sizeof(Vertex) };

    // Appending past the end of the buffer is a validation failure that aborts
    // the process, not something sokol shrugs off — so when this frame wants
    // more room than it has, skip the upload entirely. begin_frame() sees how
    // much was asked for, grows the buffer, and the next frame draws in full:
    // one dropped frame the first time a scene gets bigger than any before it.
    const bool fits = !sg_query_buffer_will_overflow(m_vbuf, data.size);
    m_frame_verts += m_verts.size();

    if (fits) {
        const int base = sg_append_buffer(m_vbuf, &data);
        for (const Batch& b : m_batches) {
            sg_bindings bind = {};
            bind.vertex_buffers[0]        = m_vbuf;
            bind.vertex_buffer_offsets[0] = base + b.start * static_cast<int>(sizeof(Vertex));
            bind.views[0]                 = b.view;
            bind.samplers[0]              = m_smp;

            sg_apply_pipeline(b.pip);
            sg_apply_bindings(&bind);
            const sg_range vp_range{ b.vp.m.data(), sizeof(float) * 16 };
            sg_apply_uniforms(0, &vp_range);
            if (!b.uniforms.empty()) {
                const sg_range u_range{ b.uniforms.data(), b.uniforms.size() };
                sg_apply_uniforms(1, &u_range);
            }
            sg_draw(0, b.count, 1);
        }
        m_draw_calls += static_cast<int>(m_batches.size());
    }

    m_verts.clear();
    m_batches.clear();
}

} // namespace loom
