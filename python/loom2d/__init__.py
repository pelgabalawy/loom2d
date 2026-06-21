"""
loom2d — cross-platform 2D game engine.
Write your game in Python; the C++ engine runs it everywhere.
"""
import sys as _sys
import os as _os

# Make the native module (loom2d_native.{pyd,so,dylib}) importable from next to
# this file, and ensure its SDL3 dependency is found:
#   - Windows: add this dir to the DLL search path (SDL3.dll lives here).
#   - Linux/macOS: the module is built with an rpath of $ORIGIN / @loader_path,
#     so libSDL3 is loaded from this same directory automatically.
_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _pkg_dir not in _sys.path:
    _sys.path.insert(0, _pkg_dir)
if hasattr(_os, "add_dll_directory"):       # Windows only
    _os.add_dll_directory(_pkg_dir)

from loom2d_native import (
    # Math
    Vec2,
    Rect,
    Color,
    # Scene
    Node,
    SpriteNode,
    Scene,
    Camera,
    # Tilemap
    Tilemap,
    TileLayer,
    Tileset,
    # Animation
    Animation,
    AnimationFrame,
    # Input
    Input,
    Key,
    MouseButton,
    # Physics
    PhysicsWorld,
    PhysicsBody,
    BodyType,
    # Audio
    AudioEngine,
    # Assets
    AssetManager,
    Texture,
    # Engine entry point
    Game,
    run,
)

__all__ = [
    "Vec2", "Rect", "Color",
    "Node", "SpriteNode", "Scene", "Camera",
    "Tilemap", "TileLayer", "Tileset",
    "Animation", "AnimationFrame",
    "Input", "Key", "MouseButton",
    "PhysicsWorld", "PhysicsBody", "BodyType",
    "AudioEngine",
    "AssetManager", "Texture",
    "Game", "run",
]
