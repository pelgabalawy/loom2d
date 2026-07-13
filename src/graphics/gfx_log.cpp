#include "graphics/gfx_log.hpp"
#include "sokol_log.h"
#include <vector>

namespace loom {
namespace {

// Only ever touched from the thread that drives sokol (the game loop), which is
// also the only thread allowed to make shaders.
std::vector<std::string> g_errors;

} // namespace

void gfx_log_func(const char* tag, uint32_t log_level, uint32_t log_item_id,
                  const char* message, uint32_t line_nr, const char* filename,
                  void* user_data) {
    // 0 = panic, 1 = error. Anything noisier (warnings, info) isn't worth
    // attaching to an exception.
    if (log_level <= 1 && message) {
        if (g_errors.size() < 32) g_errors.emplace_back(message);
    }
    slog_func(tag, log_level, log_item_id, message, line_nr, filename, user_data);
}

std::string take_gfx_errors() {
    std::string out;
    for (const std::string& e : g_errors) {
        if (!out.empty()) out += "\n";
        out += e;
    }
    g_errors.clear();
    return out;
}

} // namespace loom
