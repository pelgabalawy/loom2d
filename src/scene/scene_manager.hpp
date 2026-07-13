#pragma once
#include "scene/scene.hpp"
#include "scene/transition.hpp"
#include "graphics/camera.hpp"
#include <memory>
#include <vector>

namespace loom {

class Game;
class Renderer;

// Owns the scene stack and moves between scenes.
//
// The stack exists so a scene can be laid *over* another rather than replacing
// it: push() a pause menu and the level underneath stays alive, keeps drawing,
// and simply stops updating; pop() reveals it exactly as it was. switch_to()
// replaces the top of the stack outright — the usual menu -> level move.
//
// Every operation may be given a Transition, in which case the swap is deferred
// to the animation's midpoint (so the cut happens behind a fade) and the scene
// carries on running until then. Without one the swap is immediate.
//
// A manager always holds at least one scene, so current() is never null and
// `game.scene` works before any scene has been pushed.
class SceneManager {
public:
    SceneManager();

    // Set the owning game, and back-fill it onto scenes already on the stack.
    void set_game(Game* game);

    // ── Moving between scenes ───────────────────────────────────────────────
    // Replace the active scene.
    void switch_to(std::shared_ptr<Scene> scene,
                   std::shared_ptr<Transition> transition = nullptr);
    // Lay a scene over the active one (pause menu, dialogue box). The scene
    // below stays alive and keeps drawing, but stops updating.
    void push(std::shared_ptr<Scene> scene,
              std::shared_ptr<Transition> transition = nullptr);
    // Drop the top scene, revealing the one below. No-op if only one is left —
    // the stack is never empty.
    void pop(std::shared_ptr<Transition> transition = nullptr);

    // ── State ───────────────────────────────────────────────────────────────
    std::shared_ptr<Scene> current() const;
    std::size_t depth()          const { return m_stack.size(); }
    bool        transitioning()  const { return m_transition != nullptr; }
    std::shared_ptr<Transition> transition() const { return m_transition; }
    const std::vector<std::shared_ptr<Scene>>& stack() const { return m_stack; }

    // ── Driven by the run loop ──────────────────────────────────────────────
    // The logical screen: a scene's camera and UI are set up against this when
    // it enters, so scenes created later are laid out like the first one.
    void set_screen(int width, int height);
    // Per-frame camera viewport (changes with window size / scale mode). Applied
    // to every scene on the stack, and to scenes that enter later.
    void set_camera_viewport(int width, int height);

    // Advances the transition (swapping scenes at its midpoint) and updates the
    // top scene. Scenes below the top are paused.
    void update(float dt);
    // Draws the stack bottom to top: each scene's world, then its UI over it.
    void draw(Renderer& renderer);
    // Draws the in-flight transition over everything else. Called by the run
    // loop last, so a fade covers the game's UI layer too.
    void draw_transition(Renderer& renderer);

private:
    enum class Op { None, Replace, Push, Pop };

    void apply_pending();
    void enter(const std::shared_ptr<Scene>& scene);
    void begin(Op op, std::shared_ptr<Scene> scene,
               std::shared_ptr<Transition> transition);
    // Lay a scene's camera and UI out against the current logical screen.
    void configure(Scene& scene);

    Game*                               m_game = nullptr;
    std::vector<std::shared_ptr<Scene>> m_stack;

    std::shared_ptr<Transition> m_transition;
    std::shared_ptr<Scene>      m_pending;
    Op                          m_pending_op = Op::None;

    // Screen-space camera for the transition overlay.
    Camera m_overlay_camera;
    int    m_screen_w = 0, m_screen_h = 0;
    int    m_cam_w    = 0, m_cam_h    = 0;
};

} // namespace loom
