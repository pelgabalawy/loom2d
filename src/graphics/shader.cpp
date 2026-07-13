#include "graphics/shader.hpp"
#include "graphics/gfx_log.hpp"
#include <cstring>
#include <regex>
#include <stdexcept>

namespace loom {

// ── Pure helpers ────────────────────────────────────────────────────────────

int uniform_float_count(UniformType type) {
    switch (type) {
    case UniformType::Float: return 1;
    case UniformType::Vec2:  return 2;
    case UniformType::Vec3:  return 3;
    case UniformType::Vec4:  return 4;
    case UniformType::Mat4:  return 16;
    }
    return 0;
}

namespace {

struct Std140 { int align; int size; };

Std140 std140_of(UniformType type) {
    switch (type) {
    case UniformType::Float: return {4,  4};
    case UniformType::Vec2:  return {8,  8};
    case UniformType::Vec3:  return {16, 12};
    case UniformType::Vec4:  return {16, 16};
    case UniformType::Mat4:  return {16, 64};
    }
    return {4, 4};
}

int align_up(int value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

// A `uniform` inside a comment must not count as a declaration, and a comment is
// the one place GLSL's grammar can hide one.
std::string strip_comments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    for (size_t i = 0; i < src.size();) {
        if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
        } else if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < src.size() && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') out += '\n'; // keep line numbers honest
                ++i;
            }
            i = (i + 1 < src.size()) ? i + 2 : src.size();
        } else {
            out += src[i++];
        }
    }
    return out;
}

UniformType type_from_name(const std::string& name) {
    if (name == "float") return UniformType::Float;
    if (name == "vec2")  return UniformType::Vec2;
    if (name == "vec3")  return UniformType::Vec3;
    if (name == "vec4")  return UniformType::Vec4;
    return UniformType::Mat4;
}

const char* GLSL_VERSION_DESKTOP = "#version 410\n";
const char* GLSL_VERSION_GLES    = "#version 300 es\nprecision highp float;\n";

} // namespace

std::vector<UniformDecl> parse_uniforms(const std::string& src) {
    const std::string clean = strip_comments(src);

    // Samplers are matched too, purely so we can fail with a useful message —
    // `tex` is already declared by the wrapper, and a second sampler would have
    // no image bound to it.
    static const std::regex re(
        R"(\buniform\s+(float|vec2|vec3|vec4|mat4|sampler2D)\s+([A-Za-z_][A-Za-z0-9_]*)\s*;)");

    std::vector<UniformDecl> decls;
    int offset = 0;
    for (auto it = std::sregex_iterator(clean.begin(), clean.end(), re);
         it != std::sregex_iterator(); ++it) {
        const std::string type_name = (*it)[1].str();
        const std::string name      = (*it)[2].str();

        if (type_name == "sampler2D") {
            throw std::runtime_error(
                "shader: extra sampler uniform '" + name + "' is not supported — "
                "the drawable's own texture is already bound as 'tex'");
        }
        const UniformType type = type_from_name(type_name);
        const Std140      l    = std140_of(type);
        offset = align_up(offset, l.align);
        decls.push_back({name, type, offset});
        offset += l.size;
    }
    return decls;
}

int uniform_block_size(const std::vector<UniformDecl>& decls) {
    if (decls.empty()) return 0;
    const UniformDecl& last = decls.back();
    return align_up(last.offset + std140_of(last.type).size, 16);
}

std::string build_vertex_source(bool gles) {
    return std::string(gles ? GLSL_VERSION_GLES : GLSL_VERSION_DESKTOP) +
        "uniform mat4 u_mvp;\n"
        "in vec2 a_pos;\n"
        "in vec2 a_uv;\n"
        "in vec4 a_color;\n"
        "out vec2 v_uv;\n"
        "out vec4 v_color;\n"
        "void main(){ gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);"
        " v_uv = a_uv; v_color = a_color; }\n";
}

std::string build_fragment_source(const std::string& effect_src, bool gles) {
    // #line 1 makes the GLSL compiler number its errors against the game's own
    // source rather than the wrapper, so a reported line is the line they wrote.
    return std::string(gles ? GLSL_VERSION_GLES : GLSL_VERSION_DESKTOP) +
        "uniform sampler2D tex;\n"
        "in vec2 v_uv;\n"
        "in vec4 v_color;\n"
        "out vec4 frag_color;\n"
        "#line 1\n" +
        effect_src +
        "\n#line 10000\n"
        "void main(){ frag_color = effect(v_color, tex, v_uv); }\n";
}

// ── Shader ──────────────────────────────────────────────────────────────────

Shader::Shader(sg_shader shd, std::vector<UniformDecl> uniforms, int block_size)
    : m_shd(shd), m_uniforms(std::move(uniforms)),
      m_data(static_cast<size_t>(block_size), 0) {}

Shader::~Shader() {
    // Shaders can outlive the Renderer (a Python game may still hold one after
    // run() returns); sg_shutdown has already freed everything by then.
    if (sg_isvalid() && m_shd.id != SG_INVALID_ID) sg_destroy_shader(m_shd);
}

std::shared_ptr<Shader> Shader::from_source(const std::string& effect_src) {
    if (!sg_isvalid()) {
        throw std::runtime_error(
            "Shader: no renderer yet — create shaders in on_start() or later, "
            "not before the game window exists");
    }

    std::vector<UniformDecl> decls = parse_uniforms(effect_src);
    const int  block_size = uniform_block_size(decls);
    const bool gles       = (sg_query_backend() == SG_BACKEND_GLES3);

    const std::string vs = build_vertex_source(gles);
    const std::string fs = build_fragment_source(effect_src, gles);

    sg_shader_desc sd = {};
    sd.vertex_func.source   = vs.c_str();
    sd.fragment_func.source = fs.c_str();
    sd.attrs[0].glsl_name = "a_pos";
    sd.attrs[1].glsl_name = "a_uv";
    sd.attrs[2].glsl_name = "a_color";

    // Block 0: the view-projection, same for every shader (the batcher supplies it).
    sd.uniform_blocks[0].stage  = SG_SHADERSTAGE_VERTEX;
    sd.uniform_blocks[0].size   = sizeof(float) * 16;
    sd.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
    sd.uniform_blocks[0].glsl_uniforms[0].type      = SG_UNIFORMTYPE_MAT4;
    sd.uniform_blocks[0].glsl_uniforms[0].glsl_name = "u_mvp";

    // Block 1: whatever the game declared. sokol recomputes the std140 offsets
    // from this list in order, which is why parse_uniforms must lay them out the
    // same way.
    if (!decls.empty()) {
        if (decls.size() > static_cast<size_t>(SG_MAX_UNIFORMBLOCK_MEMBERS)) {
            throw std::runtime_error("shader: too many uniforms (max " +
                                     std::to_string(SG_MAX_UNIFORMBLOCK_MEMBERS) + ")");
        }
        sd.uniform_blocks[1].stage  = SG_SHADERSTAGE_FRAGMENT;
        sd.uniform_blocks[1].size   = static_cast<size_t>(block_size);
        sd.uniform_blocks[1].layout = SG_UNIFORMLAYOUT_STD140;
        for (size_t i = 0; i < decls.size(); ++i) {
            sg_uniform_type t = SG_UNIFORMTYPE_FLOAT;
            switch (decls[i].type) {
            case UniformType::Float: t = SG_UNIFORMTYPE_FLOAT;  break;
            case UniformType::Vec2:  t = SG_UNIFORMTYPE_FLOAT2; break;
            case UniformType::Vec3:  t = SG_UNIFORMTYPE_FLOAT3; break;
            case UniformType::Vec4:  t = SG_UNIFORMTYPE_FLOAT4; break;
            case UniformType::Mat4:  t = SG_UNIFORMTYPE_MAT4;   break;
            }
            sd.uniform_blocks[1].glsl_uniforms[i].type      = t;
            sd.uniform_blocks[1].glsl_uniforms[i].glsl_name = decls[i].name.c_str();
        }
    }

    sd.views[0].texture.stage       = SG_SHADERSTAGE_FRAGMENT;
    sd.views[0].texture.image_type  = SG_IMAGETYPE_2D;
    sd.views[0].texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT;
    sd.samplers[0].stage            = SG_SHADERSTAGE_FRAGMENT;
    sd.samplers[0].sampler_type     = SG_SAMPLERTYPE_FILTERING;
    sd.texture_sampler_pairs[0].stage        = SG_SHADERSTAGE_FRAGMENT;
    sd.texture_sampler_pairs[0].view_slot    = 0;
    sd.texture_sampler_pairs[0].sampler_slot = 0;
    sd.texture_sampler_pairs[0].glsl_name    = "tex";

    take_gfx_errors(); // drop anything unrelated that was logged earlier
    sg_shader shd = sg_make_shader(&sd);
    if (sg_query_shader_state(shd) != SG_RESOURCESTATE_VALID) {
        const std::string log = take_gfx_errors();
        if (shd.id != SG_INVALID_ID) sg_destroy_shader(shd);
        throw std::runtime_error("shader failed to compile:\n" +
                                 (log.empty() ? "(no message from the GLSL compiler)" : log));
    }

    return std::shared_ptr<Shader>(new Shader(shd, std::move(decls), block_size));
}

bool Shader::has(const std::string& name) const {
    for (const UniformDecl& d : m_uniforms) {
        if (d.name == name) return true;
    }
    return false;
}

void Shader::set(const std::string& name, const float* values, int count) {
    for (const UniformDecl& d : m_uniforms) {
        if (d.name != name) continue;

        const int expected = uniform_float_count(d.type);
        if (count != expected) {
            throw std::runtime_error("shader uniform '" + name + "' takes " +
                                     std::to_string(expected) + " float(s), got " +
                                     std::to_string(count));
        }
        std::memcpy(m_data.data() + d.offset, values, sizeof(float) * count);
        ++m_revision;
        return;
    }

    std::string known;
    for (const UniformDecl& d : m_uniforms) {
        if (!known.empty()) known += ", ";
        known += d.name;
    }
    throw std::runtime_error("shader has no uniform named '" + name + "'" +
                             (known.empty() ? " (it declares none)"
                                            : " — it declares: " + known));
}

} // namespace loom
