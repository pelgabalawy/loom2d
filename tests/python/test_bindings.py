"""
Verify that the loom2d_native module exposes the expected public API.
These tests do NOT start a window — they only inspect types and values.
"""
import pytest

# Skip entire module if the native binary isn't built yet
loom2d_native = pytest.importorskip("loom2d_native",
    reason="loom2d_native not built — run build.ps1 first")


class TestModuleExports:
    """All expected symbols are importable."""

    def test_vec2_exported(self):
        assert hasattr(loom2d_native, "Vec2")

    def test_rect_exported(self):
        assert hasattr(loom2d_native, "Rect")

    def test_color_exported(self):
        assert hasattr(loom2d_native, "Color")

    def test_game_exported(self):
        assert hasattr(loom2d_native, "Game")

    def test_run_exported(self):
        assert hasattr(loom2d_native, "run")
        assert callable(loom2d_native.run)

    def test_node_exported(self):
        assert hasattr(loom2d_native, "Node")

    def test_sprite_node_exported(self):
        assert hasattr(loom2d_native, "SpriteNode")

    def test_scene_exported(self):
        assert hasattr(loom2d_native, "Scene")

    def test_animation_exported(self):
        assert hasattr(loom2d_native, "Animation")

    def test_camera_exported(self):
        assert hasattr(loom2d_native, "Camera")

    def test_input_exported(self):
        assert hasattr(loom2d_native, "Input")

    def test_key_exported(self):
        assert hasattr(loom2d_native, "Key")

    def test_physics_world_exported(self):
        assert hasattr(loom2d_native, "PhysicsWorld")

    def test_physics_body_exported(self):
        assert hasattr(loom2d_native, "PhysicsBody")

    def test_body_type_exported(self):
        assert hasattr(loom2d_native, "BodyType")

    def test_audio_engine_exported(self):
        assert hasattr(loom2d_native, "AudioEngine")

    def test_asset_manager_exported(self):
        assert hasattr(loom2d_native, "AssetManager")

    def test_texture_exported(self):
        assert hasattr(loom2d_native, "Texture")
