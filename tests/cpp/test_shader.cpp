#include <gtest/gtest.h>
#include "graphics/blend_mode.hpp"
#include "graphics/shader.hpp"
#include <stdexcept>
#include <string>

using namespace loom;

// Everything here is the GPU-free half of Phase 2.11: the uniform parser, the
// std140 layout it produces, the GLSL the wrapper emits, and the blend table.
// Compiling a shader needs a GL context, so that lives in the Python tests.

// ── Uniform parsing ─────────────────────────────────────────────────────────

TEST(ShaderUniforms, SourceWithNoUniformsParsesEmpty) {
    auto decls = parse_uniforms("vec4 effect(vec4 c, sampler2D t, vec2 uv){ return c; }");
    EXPECT_TRUE(decls.empty());
    EXPECT_EQ(uniform_block_size(decls), 0);
}

TEST(ShaderUniforms, ParsesEachSupportedType) {
    auto decls = parse_uniforms(
        "uniform float a;\n"
        "uniform vec2  b;\n"
        "uniform vec3  c;\n"
        "uniform vec4  d;\n"
        "uniform mat4  e;\n");

    ASSERT_EQ(decls.size(), 5u);
    EXPECT_EQ(decls[0].name, "a"); EXPECT_EQ(decls[0].type, UniformType::Float);
    EXPECT_EQ(decls[1].name, "b"); EXPECT_EQ(decls[1].type, UniformType::Vec2);
    EXPECT_EQ(decls[2].name, "c"); EXPECT_EQ(decls[2].type, UniformType::Vec3);
    EXPECT_EQ(decls[3].name, "d"); EXPECT_EQ(decls[3].type, UniformType::Vec4);
    EXPECT_EQ(decls[4].name, "e"); EXPECT_EQ(decls[4].type, UniformType::Mat4);
}

TEST(ShaderUniforms, KeepsDeclarationOrder) {
    // sokol recomputes offsets by walking the declarations in order, so the
    // parser's order IS the memory layout — a reordering here would silently
    // write every uniform to the wrong place.
    auto decls = parse_uniforms("uniform vec2 second;\nuniform float first;\n");
    ASSERT_EQ(decls.size(), 2u);
    EXPECT_EQ(decls[0].name, "second");
    EXPECT_EQ(decls[1].name, "first");
}

TEST(ShaderUniforms, IgnoresUniformsInsideComments) {
    auto decls = parse_uniforms(
        "// uniform float commented_out;\n"
        "/* uniform vec2 also_out; */\n"
        "uniform float real;\n");
    ASSERT_EQ(decls.size(), 1u);
    EXPECT_EQ(decls[0].name, "real");
}

TEST(ShaderUniforms, ExtraSamplerIsRejected) {
    // There is one texture bound per draw — a second sampler would sample nothing,
    // so say that instead of emitting GLSL that fails to link.
    EXPECT_THROW(parse_uniforms("uniform sampler2D other;\n"), std::runtime_error);
}

// ── std140 layout (must match what sokol computes) ──────────────────────────

TEST(ShaderLayout, FloatsPackTightly) {
    auto decls = parse_uniforms("uniform float a;\nuniform float b;\n");
    EXPECT_EQ(decls[0].offset, 0);
    EXPECT_EQ(decls[1].offset, 4);
    EXPECT_EQ(uniform_block_size(decls), 16); // padded up to a multiple of 16
}

TEST(ShaderLayout, Vec2AlignsToEight) {
    auto decls = parse_uniforms("uniform float a;\nuniform vec2 b;\n");
    EXPECT_EQ(decls[0].offset, 0);
    EXPECT_EQ(decls[1].offset, 8); // 4 -> padded to 8
    EXPECT_EQ(uniform_block_size(decls), 16);
}

TEST(ShaderLayout, Vec3AndVec4AlignToSixteen) {
    auto decls = parse_uniforms("uniform float a;\nuniform vec3 b;\nuniform vec4 c;\n");
    EXPECT_EQ(decls[0].offset, 0);
    EXPECT_EQ(decls[1].offset, 16); // 4 -> padded to 16
    EXPECT_EQ(decls[2].offset, 32); // 16 + 12 = 28 -> padded to 32
    EXPECT_EQ(uniform_block_size(decls), 48);
}

TEST(ShaderLayout, Mat4TakesSixtyFourBytes) {
    auto decls = parse_uniforms("uniform mat4 m;\nuniform float after;\n");
    EXPECT_EQ(decls[0].offset, 0);
    EXPECT_EQ(decls[1].offset, 64);
    EXPECT_EQ(uniform_block_size(decls), 80);
}

TEST(ShaderLayout, FloatCountsMatchTypes) {
    EXPECT_EQ(uniform_float_count(UniformType::Float), 1);
    EXPECT_EQ(uniform_float_count(UniformType::Vec2),  2);
    EXPECT_EQ(uniform_float_count(UniformType::Vec3),  3);
    EXPECT_EQ(uniform_float_count(UniformType::Vec4),  4);
    EXPECT_EQ(uniform_float_count(UniformType::Mat4),  16);
}

// ── GLSL assembly ───────────────────────────────────────────────────────────

TEST(ShaderSource, DesktopAndMobileGetTheirOwnVersionHeader) {
    // The same game source has to compile on GL core and GLES3 — that is the
    // whole reason the game writes effect() instead of a full program.
    const std::string effect = "vec4 effect(vec4 c, sampler2D t, vec2 uv){ return c; }";

    const std::string desktop = build_fragment_source(effect, /*gles=*/false);
    EXPECT_NE(desktop.find("#version 410"), std::string::npos);
    EXPECT_EQ(desktop.find("precision"), std::string::npos);

    const std::string mobile = build_fragment_source(effect, /*gles=*/true);
    EXPECT_NE(mobile.find("#version 300 es"), std::string::npos);
    EXPECT_NE(mobile.find("precision highp float"), std::string::npos);
}

TEST(ShaderSource, FragmentWrapperDeclaresTheEffectInterfaceAndCallsIt) {
    const std::string fs = build_fragment_source("vec4 effect(vec4 c, sampler2D t, vec2 uv){ return c; }",
                                                 /*gles=*/false);
    EXPECT_NE(fs.find("uniform sampler2D tex;"), std::string::npos);
    EXPECT_NE(fs.find("in vec2 v_uv;"),          std::string::npos);
    EXPECT_NE(fs.find("in vec4 v_color;"),       std::string::npos);
    EXPECT_NE(fs.find("effect(v_color, tex, v_uv)"), std::string::npos);
}

TEST(ShaderSource, GameSourceIsEmbeddedVerbatim) {
    const std::string effect = "uniform float u_time;\nvec4 effect(vec4 c, sampler2D t, vec2 uv){ return c; }";
    EXPECT_NE(build_fragment_source(effect, false).find(effect), std::string::npos);
}

TEST(ShaderSource, VertexShaderExposesTheAttributesTheBatcherSends) {
    const std::string vs = build_vertex_source(/*gles=*/false);
    EXPECT_NE(vs.find("in vec2 a_pos;"),   std::string::npos);
    EXPECT_NE(vs.find("in vec2 a_uv;"),    std::string::npos);
    EXPECT_NE(vs.find("in vec4 a_color;"), std::string::npos);
    EXPECT_NE(vs.find("u_mvp"),            std::string::npos);
}

// ── Blend modes ─────────────────────────────────────────────────────────────

TEST(BlendModes, AlphaIsStandardSourceOver) {
    const sg_blend_state bs = blend_state_for(BlendMode::Alpha);
    EXPECT_TRUE(bs.enabled);
    EXPECT_EQ(bs.src_factor_rgb, SG_BLENDFACTOR_SRC_ALPHA);
    EXPECT_EQ(bs.dst_factor_rgb, SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
}

TEST(BlendModes, AddKeepsTheDestinationAndScalesByAlpha) {
    // dst_factor ONE is what makes it additive; SRC_ALPHA on the source is what
    // lets a game fade a glow out by dropping the sprite's alpha.
    const sg_blend_state bs = blend_state_for(BlendMode::Add);
    EXPECT_TRUE(bs.enabled);
    EXPECT_EQ(bs.src_factor_rgb, SG_BLENDFACTOR_SRC_ALPHA);
    EXPECT_EQ(bs.dst_factor_rgb, SG_BLENDFACTOR_ONE);
}

TEST(BlendModes, MultiplyScalesTheDestinationBySource) {
    const sg_blend_state bs = blend_state_for(BlendMode::Multiply);
    EXPECT_TRUE(bs.enabled);
    EXPECT_EQ(bs.src_factor_rgb, SG_BLENDFACTOR_DST_COLOR);
    EXPECT_EQ(bs.dst_factor_rgb, SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
}

TEST(BlendModes, ScreenIsTheInverseOfMultiply) {
    const sg_blend_state bs = blend_state_for(BlendMode::Screen);
    EXPECT_TRUE(bs.enabled);
    EXPECT_EQ(bs.src_factor_rgb, SG_BLENDFACTOR_ONE);
    EXPECT_EQ(bs.dst_factor_rgb, SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR);
}

TEST(BlendModes, ReplaceTurnsBlendingOff) {
    EXPECT_FALSE(blend_state_for(BlendMode::Replace).enabled);
}

TEST(BlendModes, EveryModeAddsRatherThanSubtracts) {
    for (BlendMode mode : {BlendMode::Alpha, BlendMode::Add, BlendMode::Multiply,
                           BlendMode::Screen}) {
        const sg_blend_state bs = blend_state_for(mode);
        EXPECT_EQ(bs.op_rgb,   SG_BLENDOP_ADD);
        EXPECT_EQ(bs.op_alpha, SG_BLENDOP_ADD);
    }
}
