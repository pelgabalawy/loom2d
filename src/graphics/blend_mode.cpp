#include "graphics/blend_mode.hpp"

namespace loom {

sg_blend_state blend_state_for(BlendMode mode) {
    sg_blend_state bs = {};
    bs.enabled  = true;
    bs.op_rgb   = SG_BLENDOP_ADD;
    bs.op_alpha = SG_BLENDOP_ADD;

    switch (mode) {
    case BlendMode::Alpha:
        // out.rgb = src.rgb*src.a + dst.rgb*(1-src.a)
        bs.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
        bs.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        bs.src_factor_alpha = SG_BLENDFACTOR_ONE;
        bs.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        break;

    case BlendMode::Add:
        // out.rgb = src.rgb*src.a + dst.rgb — fading a sprite's alpha fades the
        // glow out instead of leaving it at full strength. Target alpha is kept.
        bs.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
        bs.dst_factor_rgb   = SG_BLENDFACTOR_ONE;
        bs.src_factor_alpha = SG_BLENDFACTOR_ZERO;
        bs.dst_factor_alpha = SG_BLENDFACTOR_ONE;
        break;

    case BlendMode::Multiply:
        // out.rgb = dst.rgb*src.rgb + dst.rgb*(1-src.a). The second term makes a
        // transparent pixel leave the target alone rather than crushing it to
        // black, so alpha still fades the effect in and out.
        bs.src_factor_rgb   = SG_BLENDFACTOR_DST_COLOR;
        bs.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        bs.src_factor_alpha = SG_BLENDFACTOR_ZERO;
        bs.dst_factor_alpha = SG_BLENDFACTOR_ONE;
        break;

    case BlendMode::Screen:
        // out.rgb = src.rgb + dst.rgb*(1-src.rgb) — the inverse of multiply.
        bs.src_factor_rgb   = SG_BLENDFACTOR_ONE;
        bs.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
        bs.src_factor_alpha = SG_BLENDFACTOR_ONE;
        bs.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        break;

    case BlendMode::Replace:
        bs.enabled = false;
        break;
    }
    return bs;
}

} // namespace loom
