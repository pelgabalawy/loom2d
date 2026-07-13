"""
Shaders, blend modes and canvases (Phase 2.11) — the parts testable without a
GPU: the Python surface, the defaults, and the errors a game gets when it builds
a shader or canvas too early.

Compiling GLSL and rendering to a canvas need a real GL context, which the test
runner has no window for. Those paths are exercised by examples/shaders, which
renders them in a real window and self-terminates (--frames).
"""
import pytest

loom = pytest.importorskip("loom2d")


class TestExports:
    def test_rendering_types_are_exported(self):
        assert loom.Shader is not None
        assert loom.Canvas is not None
        assert loom.BlendMode is not None

    def test_blend_modes(self):
        for name in ("Alpha", "Add", "Multiply", "Screen", "Replace"):
            assert hasattr(loom.BlendMode, name)


class TestDrawState:
    """shader/blend live on the drawable, and default to 'draw it normally'."""

    def test_node_defaults(self):
        node = loom.Node()
        assert node.shader is None
        assert node.blend == loom.BlendMode.Alpha

    def test_node_blend_is_settable(self):
        node = loom.Node()
        node.blend = loom.BlendMode.Add
        assert node.blend == loom.BlendMode.Add

    def test_widget_defaults(self):
        widget = loom.Widget()
        assert widget.shader is None
        assert widget.blend == loom.BlendMode.Alpha

    def test_widget_blend_is_settable(self):
        panel = loom.Panel()
        panel.blend = loom.BlendMode.Multiply
        assert panel.blend == loom.BlendMode.Multiply

    def test_blend_survives_a_python_subclass(self):
        class Glow(loom.Node):
            def __init__(self):
                super().__init__()
                self.blend = loom.BlendMode.Add

        assert Glow().blend == loom.BlendMode.Add


class TestGameSurface:
    def test_post_process_defaults_to_none(self):
        assert loom.Game().post_process is None

    def test_render_to_canvas_is_available(self):
        assert callable(loom.Game().render_to_canvas)


class TestNeedsAWindow:
    """A shader or canvas built before the window exists must say so, not crash.

    The GPU objects behind them can only be made once sokol is set up, and the
    natural mistake is to build one at module scope.
    """

    def test_shader_without_a_renderer_raises(self):
        with pytest.raises(RuntimeError, match="no renderer yet"):
            loom.Shader("vec4 effect(vec4 c, sampler2D t, vec2 uv){ return c; }")

    def test_canvas_without_a_renderer_raises(self):
        with pytest.raises(RuntimeError, match="no renderer yet"):
            loom.Canvas(64, 64)
