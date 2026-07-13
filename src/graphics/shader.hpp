#pragma once
#include "sokol_gfx.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace loom {

// A custom fragment shader, in the LÖVE style: the game writes an `effect()`
// function, not a whole GLSL program.
//
//     uniform float u_time;
//     vec4 effect(vec4 color, sampler2D tex, vec2 uv) {
//         vec4 c = texture(tex, uv);
//         c.rgb *= 0.5 + 0.5 * sin(u_time);
//         return c * color;
//     }
//
// loom2d wraps that in the correct header for whichever GLSL dialect the backend
// wants (`#version 410` on desktop, `#version 300 es` + precision on mobile), so
// one shader source runs everywhere. `color` is the drawable's tint, `uv` the
// texture coordinate; write to the return value instead of gl_FragColor.
//
// Any `uniform` the source declares can be set from the game with set().

enum class UniformType { Float, Vec2, Vec3, Vec4, Mat4 };

struct UniformDecl {
    std::string name;
    UniformType type;
    int         offset; // byte offset into the std140 uniform block
};

// ── Pure helpers (no GPU — unit-tested) ─────────────────────────────────────

// Number of floats a uniform of this type holds (Mat4 = 16).
int uniform_float_count(UniformType type);

// Scan GLSL for top-level `uniform <type> <name>;` declarations, in source
// order, assigning each a std140 byte offset. The offsets must match the ones
// sokol computes from the same declaration order, so this mirrors its rules:
// float aligns to 4, vec2 to 8, vec3/vec4/mat4 to 16.
// Throws if the source declares a sampler (only the built-in `tex` is bound).
std::vector<UniformDecl> parse_uniforms(const std::string& src);

// Size of the std140 block holding these uniforms, padded up to a multiple of
// 16 (sokol asserts on the exact value).
int uniform_block_size(const std::vector<UniformDecl>& decls);

// Wrap a user `effect()` body in the full GLSL program for the backend.
std::string build_fragment_source(const std::string& effect_src, bool gles);
std::string build_vertex_source(bool gles);

// ── Shader ──────────────────────────────────────────────────────────────────

class Shader {
public:
    // Compiles immediately, so a Renderer must exist. Throws std::runtime_error
    // with the GLSL compiler's message if the source doesn't build.
    static std::shared_ptr<Shader> from_source(const std::string& effect_src);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Set a uniform declared by the source. `count` is how many floats `values`
    // holds; it must match the declared type. Throws if the name isn't declared
    // or the arity is wrong.
    void set(const std::string& name, const float* values, int count);

    bool has(const std::string& name) const;
    const std::vector<UniformDecl>& uniforms() const { return m_uniforms; }

    sg_shader handle() const { return m_shd; }

    // The packed std140 block, ready to hand to sg_apply_uniforms. Empty when
    // the source declares no uniforms.
    const std::vector<uint8_t>& uniform_data() const { return m_data; }

    // Bumped by every set(). The batcher snapshots uniform_data() when it opens
    // a batch and compares revisions to decide whether a later quad can join it
    // — so the same shader can draw twice in one frame with different uniforms.
    uint32_t revision() const { return m_revision; }

private:
    Shader(sg_shader shd, std::vector<UniformDecl> uniforms, int block_size);

    sg_shader                m_shd = {};
    std::vector<UniformDecl> m_uniforms;
    std::vector<uint8_t>     m_data;
    uint32_t                 m_revision = 1;
};

} // namespace loom
