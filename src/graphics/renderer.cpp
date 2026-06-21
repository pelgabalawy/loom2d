#include "graphics/renderer.hpp"
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
