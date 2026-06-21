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

// ── Shader sources ──────────────────────────────────────────────────────────

static const char* VS_GLCORE =
    "#version 410\n"
    "uniform mat4 u_mvp;\n"
    "in vec2 a_pos;\n"
    "in vec2 a_uv;\n"
    "in vec4 a_color;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_color;\n"
    "void main(){ gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0); v_uv = a_uv; v_color = a_color; }\n";

static const char* FS_GLCORE =
    "#version 410\n"
    "uniform sampler2D tex;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main(){ frag_color = texture(tex, v_uv) * v_color; }\n";

static const char* VS_GLES3 =
    "#version 300 es\n"
    "uniform mat4 u_mvp;\n"
    "in vec2 a_pos;\n"
    "in vec2 a_uv;\n"
    "in vec4 a_color;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_color;\n"
    "void main(){ gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0); v_uv = a_uv; v_color = a_color; }\n";

static const char* FS_GLES3 =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main(){ frag_color = texture(tex, v_uv) * v_color; }\n";

// ── SpriteBatcher ───────────────────────────────────────────────────────────

SpriteBatcher::~SpriteBatcher() {
    // sg_shutdown() (called by ~Renderer) frees everything; only clean up
    // explicitly if sokol is still alive when this batcher dies.
    if (m_ready && sg_isvalid()) {
        sg_destroy_pipeline(m_pip);
        sg_destroy_shader(m_shd);
        sg_destroy_sampler(m_smp);
        if (m_vbuf.id != SG_INVALID_ID) sg_destroy_buffer(m_vbuf);
    }
}

void SpriteBatcher::init() {
    bool gles = (sg_query_backend() == SG_BACKEND_GLES3);

    sg_shader_desc sd = {};
    sd.vertex_func.source   = gles ? VS_GLES3 : VS_GLCORE;
    sd.fragment_func.source = gles ? FS_GLES3 : FS_GLCORE;
    sd.attrs[0].glsl_name = "a_pos";
    sd.attrs[1].glsl_name = "a_uv";
    sd.attrs[2].glsl_name = "a_color";
    sd.uniform_blocks[0].stage  = SG_SHADERSTAGE_VERTEX;
    sd.uniform_blocks[0].size   = sizeof(float) * 16;
    sd.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
    sd.uniform_blocks[0].glsl_uniforms[0].type      = SG_UNIFORMTYPE_MAT4;
    sd.uniform_blocks[0].glsl_uniforms[0].glsl_name = "u_mvp";
    sd.views[0].texture.stage       = SG_SHADERSTAGE_FRAGMENT;
    sd.views[0].texture.image_type  = SG_IMAGETYPE_2D;
    sd.views[0].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
    sd.samplers[0].stage        = SG_SHADERSTAGE_FRAGMENT;
    sd.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
    sd.texture_sampler_pairs[0].stage        = SG_SHADERSTAGE_FRAGMENT;
    sd.texture_sampler_pairs[0].view_slot    = 0;
    sd.texture_sampler_pairs[0].sampler_slot = 0;
    sd.texture_sampler_pairs[0].glsl_name    = "tex";
    m_shd = sg_make_shader(&sd);

    sg_pipeline_desc pd = {};
    pd.shader = m_shd;
    pd.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2; // a_pos
    pd.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // a_uv
    pd.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4; // a_color
    pd.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pd.colors[0].blend.enabled        = true;
    pd.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pd.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pd.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pd.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    m_pip = sg_make_pipeline(&pd);

    sg_sampler_desc smp = {};
    smp.min_filter = SG_FILTER_NEAREST;
    smp.mag_filter = SG_FILTER_NEAREST;
    smp.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    smp.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    m_smp = sg_make_sampler(&smp);

    m_ready = true;
    ensure_capacity(4096);
}

void SpriteBatcher::ensure_capacity(size_t need) {
    if (m_vbuf.id != SG_INVALID_ID && need <= m_capacity) return;
    if (m_vbuf.id != SG_INVALID_ID) sg_destroy_buffer(m_vbuf);

    size_t cap = m_capacity ? m_capacity : 4096;
    while (cap < need) cap *= 2;

    sg_buffer_desc bd = {};
    bd.size = cap * sizeof(Vertex);
    bd.usage.vertex_buffer  = true;
    bd.usage.dynamic_update = true;
    m_vbuf = sg_make_buffer(&bd);
    m_capacity = cap;
}

void SpriteBatcher::submit(const Texture& texture, const SpriteQuad& q,
                           const Color& tint) {
    sg_view view = texture.view();
    static const int idx[6] = {0, 1, 2, 0, 2, 3}; // two triangles of TL,TR,BR,BL

    if (!m_batches.empty() && m_batches.back().view.id == view.id) {
        m_batches.back().count += 6;
    } else {
        m_batches.push_back({view, static_cast<int>(m_verts.size()), 6});
    }
    for (int i = 0; i < 6; ++i) {
        int k = idx[i];
        m_verts.push_back({ q.pos[k].x, q.pos[k].y, q.uv[k][0], q.uv[k][1],
                            tint.r, tint.g, tint.b, tint.a });
    }
}

void SpriteBatcher::flush() {
    if (m_verts.empty()) { m_draw_calls = 0; return; }

    ensure_capacity(m_verts.size());
    sg_range data{ m_verts.data(), m_verts.size() * sizeof(Vertex) };
    sg_update_buffer(m_vbuf, &data);

    sg_apply_pipeline(m_pip);
    sg_range vp_range{ m_vp.m.data(), sizeof(float) * 16 };

    for (const Batch& b : m_batches) {
        sg_bindings bind = {};
        bind.vertex_buffers[0]        = m_vbuf;
        bind.vertex_buffer_offsets[0] = b.start * static_cast<int>(sizeof(Vertex));
        bind.views[0]                 = b.view;
        bind.samplers[0]              = m_smp;
        sg_apply_bindings(&bind);
        sg_apply_uniforms(0, &vp_range);
        sg_draw(0, b.count, 1);
    }

    m_draw_calls = static_cast<int>(m_batches.size());
    m_verts.clear();
    m_batches.clear();
}

} // namespace loom
