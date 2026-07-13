#pragma once
#include "graphics/color.hpp"
#include "graphics/scaling.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_canvas.hpp"
#include "physics/physics.hpp"
#include "audio/audio.hpp"
#include "assets/asset_manager.hpp"
#include <memory>
#include <string>

namespace loom {

// The game: every subsystem, plus the hooks a game overrides.
//
// Subclass it (in C++ or Python), override the on_* hooks, and hand it to run().
class Game {
public:
    Game();
    virtual ~Game() = default;

    Color        clear_color = Color::cornflower();
    SceneManager scenes;
    UICanvas     ui;        // screen-space UI drawn over every scene
    PhysicsWorld physics;
    AudioEngine  audio;
    AssetManager assets;

    // The active scene. Never null — the manager always holds at least one — so
    // a game that never touches `scenes` can still just use `scene`.
    Scene&                 scene()     { return *scenes.current(); }
    std::shared_ptr<Scene> scene_ptr() { return scenes.current(); }

    // Step physics & scenes automatically each frame; override for custom logic
    bool auto_physics = true;
    bool auto_scene   = true;

    // Set to false from on_update() to exit the game loop programmatically.
    bool running = true;
    // Number of draw calls emitted in the previous frame (diagnostics).
    int  last_draw_calls = 0;

    // ── Responsive scaling (Phase 2.7) ──────────────────────────────────────
    // Logical (design) resolution the game is authored against. 0 means "use the
    // initial window size"; run() resolves it to a concrete value on startup.
    int       logical_width  = 0;
    int       logical_height = 0;
    // How that logical resolution maps onto the real (possibly resized / HiDPI)
    // drawable surface. Fit (letterbox, preserve aspect) by default.
    ScaleMode scale_mode = ScaleMode::Fit;
    // Current drawable size in device pixels, updated by run() every frame.
    int       screen_width  = 0;
    int       screen_height = 0;

    virtual void on_start()              {}
    virtual void on_update(float dt)     { (void)dt; }
    virtual void on_draw()               {}
    virtual void on_stop()               {}
    // Called whenever the drawable surface changes size (resize / DPI change).
    virtual void on_resize(int w, int h) { (void)w; (void)h; }
};

// Open a window and run the game loop until the game stops.
void run_game(Game& game, const std::string& title, int width, int height);

} // namespace loom
