#include "scene/scene_manager.hpp"
#include "graphics/renderer.hpp"
#include "graphics/sprite_batcher.hpp"

namespace loom {

// Start with an empty scene so current() is never null: a game that never
// touches the manager still has `game.scene` to add nodes to.
SceneManager::SceneManager() {
    m_stack.push_back(std::make_shared<Scene>());
}

void SceneManager::set_game(Game* game) {
    m_game = game;
    for (auto& scene : m_stack)
        scene->game = game;
}

std::shared_ptr<Scene> SceneManager::current() const {
    return m_stack.empty() ? nullptr : m_stack.back();
}

// ── Layout ──────────────────────────────────────────────────────────────────

void SceneManager::configure(Scene& scene) {
    // Match the run loop's startup convention: world (0,0) is the top-left of
    // the screen, so a scene created mid-game is laid out like the first one.
    scene.camera.set_viewport(m_screen_w, m_screen_h);
    scene.camera.set_position(Vec2(m_screen_w * 0.5f, m_screen_h * 0.5f));
    scene.ui.set_screen(m_screen_w, m_screen_h);

    // The live viewport may differ from the logical one (window resize, Expand
    // scale mode), so re-apply it if the run loop has told us about it already.
    if (m_cam_w > 0 && m_cam_h > 0)
        scene.camera.set_viewport(m_cam_w, m_cam_h);
}

void SceneManager::set_screen(int width, int height) {
    m_screen_w = width;
    m_screen_h = height;
    m_overlay_camera.set_viewport(width, height);
    m_overlay_camera.set_position(Vec2(width * 0.5f, height * 0.5f));

    for (auto& scene : m_stack)
        configure(*scene);
}

void SceneManager::set_camera_viewport(int width, int height) {
    m_cam_w = width;
    m_cam_h = height;
    for (auto& scene : m_stack)
        scene->camera.set_viewport(width, height);
}

// ── Moving between scenes ───────────────────────────────────────────────────

void SceneManager::enter(const std::shared_ptr<Scene>& scene) {
    // Wire the scene up before on_enter() runs, so the hook can reach the game's
    // assets/audio and can switch scenes again straight away.
    scene->game = m_game;
    configure(*scene);
    scene->on_enter();
}

void SceneManager::begin(Op op, std::shared_ptr<Scene> scene,
                         std::shared_ptr<Transition> transition) {
    // A second move requested mid-transition: settle the one in flight before
    // starting the new one, so the stack is never left half-swapped.
    if (m_transition && m_pending_op != Op::None)
        apply_pending();

    m_pending_op = op;
    m_pending    = std::move(scene);
    m_transition = std::move(transition);

    // Without a transition the move happens now; with one it waits for the
    // animation's midpoint, so the cut lands behind the fade.
    if (!m_transition)
        apply_pending();
}

void SceneManager::apply_pending() {
    switch (m_pending_op) {
    case Op::Replace:
        if (!m_stack.empty()) {
            m_stack.back()->on_exit();
            m_stack.pop_back();
        }
        m_stack.push_back(m_pending);
        enter(m_pending);
        break;

    case Op::Push:
        m_stack.push_back(m_pending);
        enter(m_pending);
        break;

    case Op::Pop:
        // Never pop the last scene — current() must stay valid.
        if (m_stack.size() > 1) {
            m_stack.back()->on_exit();
            m_stack.pop_back();
        }
        break;

    case Op::None:
        break;
    }

    m_pending_op = Op::None;
    m_pending.reset();
}

void SceneManager::switch_to(std::shared_ptr<Scene> scene,
                             std::shared_ptr<Transition> transition) {
    if (!scene) return;
    begin(Op::Replace, std::move(scene), std::move(transition));
}

void SceneManager::push(std::shared_ptr<Scene> scene,
                        std::shared_ptr<Transition> transition) {
    if (!scene) return;
    begin(Op::Push, std::move(scene), std::move(transition));
}

void SceneManager::pop(std::shared_ptr<Transition> transition) {
    begin(Op::Pop, nullptr, std::move(transition));
}

// ── Per-frame ───────────────────────────────────────────────────────────────

void SceneManager::update(float dt) {
    if (m_transition) {
        m_transition->update(dt);

        if (m_pending_op != Op::None && m_transition->swap_ready())
            apply_pending();

        if (m_transition->done())
            m_transition.reset();
    }

    // Only the top scene runs; anything underneath is paused, which is the whole
    // point of push() (a pause menu freezes the level below it).
    if (auto scene = current())
        scene->update(dt);
}

void SceneManager::draw(Renderer& renderer) {
    // Bottom to top, so a pushed overlay lands over the scene it covers — and
    // that scene's HUD still shows through behind it.
    for (auto& scene : m_stack) {
        scene->draw(renderer);
        renderer.batcher().set_view_projection(scene->ui.camera.view_projection());
        scene->ui.draw(renderer);
    }
}

void SceneManager::draw_transition(Renderer& renderer) {
    if (!m_transition) return;

    renderer.batcher().set_view_projection(m_overlay_camera.view_projection());
    m_transition->draw(renderer, m_screen_w, m_screen_h);
}

} // namespace loom
