#include "graphics/renderer.hpp"
#include "graphics/texture.hpp"
#include "sokol_gfx.h"
#include "sokol_log.h"

namespace loom {

Renderer::Renderer(Window& window) : m_window(window) {
    sg_desc desc = {};
    desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    desc.environment.defaults.sample_count = 1;
    desc.logger.func = slog_func;
    sg_setup(&desc);

    m_batcher.init();
}

Renderer::~Renderer() {
    sg_shutdown();
}

void Renderer::begin_frame(const Color& clear) {
    sg_pass pass = {};
    pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass.action.colors[0].clear_value = { clear.r, clear.g, clear.b, clear.a };
    pass.swapchain.width        = m_window.drawable_width();
    pass.swapchain.height       = m_window.drawable_height();
    pass.swapchain.sample_count = 1;
    pass.swapchain.color_format = SG_PIXELFORMAT_RGBA8;
    pass.swapchain.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    pass.swapchain.gl.framebuffer = 0; // default framebuffer
    sg_begin_pass(&pass);
    m_batcher.reset_draw_calls(); // counter accumulates across this frame's flushes
}

void Renderer::set_viewport(int x, int y, int w, int h) {
    // origin_top_left = true matches our screen-space (y-down) viewport rects.
    sg_apply_viewport(x, y, w, h, true);
    sg_apply_scissor_rect(x, y, w, h, true);
}

const Texture& Renderer::white_texture() {
    if (!m_white) {
        const unsigned char px[4] = {255, 255, 255, 255};
        m_white = Texture::from_memory(px, 1, 1);
    }
    return *m_white;
}

void Renderer::fill_rect(const Rect& dst, const Color& color) {
    draw_texture(white_texture(), dst, color, {});
}

void Renderer::draw_texture(const Texture& texture, const Rect& dst,
                            const Color& tint, Rect src) {
    float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
    if (src.w > 0.f && src.h > 0.f && texture.width() > 0 && texture.height() > 0) {
        u0 = src.x / texture.width();  u1 = (src.x + src.w) / texture.width();
        v0 = src.y / texture.height(); v1 = (src.y + src.h) / texture.height();
    }
    // Axis-aligned screen quad, corner order TL, TR, BR, BL (matches the batcher).
    SpriteQuad q;
    q.pos[0] = {dst.x,         dst.y};
    q.pos[1] = {dst.x + dst.w, dst.y};
    q.pos[2] = {dst.x + dst.w, dst.y + dst.h};
    q.pos[3] = {dst.x,         dst.y + dst.h};
    q.uv[0][0] = u0; q.uv[0][1] = v0;
    q.uv[1][0] = u1; q.uv[1][1] = v0;
    q.uv[2][0] = u1; q.uv[2][1] = v1;
    q.uv[3][0] = u0; q.uv[3][1] = v1;
    m_batcher.submit(texture, q, tint);
}

void Renderer::end_frame() {
    m_batcher.flush();
    sg_end_pass();
    sg_commit();
    m_window.present();
}

int Renderer::width()  const { return m_window.width();  }
int Renderer::height() const { return m_window.height(); }

} // namespace loom
