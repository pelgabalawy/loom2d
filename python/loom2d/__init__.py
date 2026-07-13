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
    SceneManager,
    Transition,
    Fade,
    Camera,
    ScaleMode,
    # Tilemap
    Tilemap,
    TileLayer,
    Tileset,
    # Text
    Font,
    TextNode,
    TextAlign,
    # UI
    Widget,
    Panel,
    Label,
    Button,
    Image,
    Grid,
    UICanvas,
    # Animation
    Animation,
    AnimationFrame,
    # Timers & tweens
    Timers,
    Tween,
    TweenManager,
    Ease,
    ease,
    # Input
    Input,
    Key,
    MouseButton,
    GamepadButton,
    GamepadAxis,
    TouchPoint,
    # Physics
    PhysicsWorld,
    PhysicsBody,
    BodyType,
    ContactPair,
    SensorPair,
    RaycastHit,
    # Audio
    AudioEngine,
    SoundHandle,
    # Assets
    AssetManager,
    Texture,
    # Engine entry point
    Game,
    run,
)

# Save files are plain JSON, so they are written in Python rather than C++ —
# save_dir() is the only part the OS has to answer.
from .save import SaveFile, save_dir

__all__ = [
    "Vec2", "Rect", "Color",
    "Node", "SpriteNode", "Scene", "SceneManager", "Transition", "Fade",
    "Camera", "ScaleMode",
    "Tilemap", "TileLayer", "Tileset",
    "Font", "TextNode", "TextAlign",
    "Widget", "Panel", "Label", "Button", "Image", "Grid", "UICanvas",
    "Animation", "AnimationFrame",
    "Timers", "Tween", "TweenManager", "Ease", "ease",
    "SaveFile", "save_dir",
    "Input", "Key", "MouseButton", "GamepadButton", "GamepadAxis", "TouchPoint",
    "PhysicsWorld", "PhysicsBody", "BodyType",
    "ContactPair", "SensorPair", "RaycastHit",
    "AudioEngine", "SoundHandle",
    "AssetManager", "Texture",
    "Game", "run",
]
