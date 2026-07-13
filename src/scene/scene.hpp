#pragma once
#include "scene/node.hpp"
#include "graphics/camera.hpp"
#include "ui/ui_canvas.hpp"
#include <memory>

namespace loom {

class Game;

// A scene is both a container and a game state.
//
// As a container it owns a node tree, a camera and its own screen-space UI layer
// (so a menu's buttons and a level's HUD don't have to share one canvas). As a
// state it has lifecycle hooks a subclass overrides — on_enter when it becomes
// active, on_update every frame, on_exit when it is replaced or popped.
//
// The SceneManager owns scenes and drives all of this; see scene_manager.hpp.
class Scene {
public:
    Camera   camera;
    UICanvas ui;

    // The game this scene belongs to — set by the SceneManager when the scene is
    // handed to it, so a scene can reach assets/audio/physics and switch scenes.
    // Null until then.
    Game* game = nullptr;

    Scene();
    virtual ~Scene() = default;

    // ── Node tree ───────────────────────────────────────────────────────────
    void add(std::shared_ptr<Node> node);
    void remove(Node* node);
    void clear();

    // ── Lifecycle (override these) ──────────────────────────────────────────
    // Called when the scene becomes active. Build the scene here rather than in
    // __init__, so that `game` is available.
    virtual void on_enter() {}
    // Called when the scene is replaced or popped, before it is released.
    virtual void on_exit() {}
    // Called once a frame while the scene is on top of the stack.
    virtual void on_update(float dt) { (void)dt; }
    // Called once a frame after the scene's nodes are drawn.
    virtual void on_draw() {}

    // ── Driven by the SceneManager ──────────────────────────────────────────
    // Runs on_update(), then updates the node tree.
    void update(float dt);
    // Draws the node tree through this scene's camera. The manager draws the
    // scene's UI layer separately, on top, in screen space.
    void draw(Renderer& renderer);

    // C++ convenience — returns a reference into the shared_ptr root
    Node& root() { return *m_root; }

    // Python-safe accessor — returns the shared_ptr itself
    std::shared_ptr<Node> root_ptr() { return m_root; }

private:
    std::shared_ptr<Node> m_root;
};

} // namespace loom
