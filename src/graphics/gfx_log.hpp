#pragma once
#include <cstdint>
#include <string>

namespace loom {

// sokol_gfx reports GLSL compile/link failures through its logger and then just
// hands back an invalid shader object. Without capturing the log, a typo in a
// game's shader surfaces as "shader creation failed" with the actual GLSL error
// lost on stderr. Renderer installs this as sokol's log function: it forwards
// everything to slog_func (so the normal console output is unchanged) and keeps
// the recent error-level messages so Shader can put them in the exception.
void gfx_log_func(const char* tag, uint32_t log_level, uint32_t log_item_id,
                  const char* message, uint32_t line_nr, const char* filename,
                  void* user_data);

// Errors captured since the last call, joined by newlines. Clears the buffer.
std::string take_gfx_errors();

} // namespace loom
