#pragma once
#include "sokol_gfx.h"

namespace loom {

// How a drawable's colour is combined with what is already in the render target.
enum class BlendMode {
    Alpha,    // default — normal transparency (src over dst)
    Add,      // additive — glows, fire, lasers, lightning
    Multiply, // darkens — shadows, colour washes
    Screen,   // lightens — soft glows that never blow out to white
    Replace,  // no blending — the drawable overwrites the target, alpha included
};

// Pure mapping from a BlendMode to a sokol blend state. No GPU calls, so the
// factor table is unit-tested directly.
sg_blend_state blend_state_for(BlendMode mode);

} // namespace loom
