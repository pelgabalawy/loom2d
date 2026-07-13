#pragma once
#include "graphics/canvas.hpp"
#include "graphics/color.hpp"
#include "graphics/scaling.hpp"
#include "graphics/shader.hpp"
#include "core/timers.hpp"
#include "core/tween.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_canvas.hpp"
#include "physics/physics.hpp"
#include "audio/audio.hpp"
#include "assets/asset_manager.hpp"
#include <memory>
#include <string>

namespace loom {

class Renderer;

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
    Timers       timers;    // run a callback later, or on a repeat
    TweenManager tweens;    // move a value from A to B over time
    PhysicsWorld physics;
    AudioEngine  audio;
    AssetManager assets;

    // The active scene. Never null — the manager always holds at least one — so
    // a game that never touches `scenes` can still just use `scene`.
    Scene&                 scene()     { return *scenes.current(); }
    std::shared_ptr<Scene> scene_ptr() { return scenes.current(); }

    // Step these automatically each frame; turn one off to drive it yourself.
    bool auto_physics = true;
    bool auto_scene   = true;
    bool auto_timers  = true;
    bool auto_tweens  = true;

    // Set to false from on_update() to exit the game loop programmatically.
    bool running = true;
    // Number of draw calls emitted in the previous frame (diagnostics).
    int  last_draw_calls = 0;

    // ── Screen shader (Phase 2.11) ──────────────────────────────────────────
    // A shader applied to the whole finished frame — CRT, colour grade, shake,
    // vignette. The run loop renders everything into a canvas the size of the
    // window, then draws that canvas through this shader. Null = draw straight
    // to the window, with no extra pass.
    std::shared_ptr<Shader> post_process;

    // Draw a node and its children into a canvas, which can then be used as an
    // ordinary texture (minimap, portal, security camera, a baked backdrop).
    // With no camera, the canvas is drawn in its own pixel space: (0,0) is its
    // top-left corner.
    //
    // Call this from on_update(). A canvas is its own render pass, and passes
    // cannot nest — from inside on_draw() the frame's pass is already open, and
    // this throws to say so.
    void render_to_canvas(Canvas& canvas, Node& root);
    void render_to_canvas(Canvas& canvas, Node& root, const Camera& camera);

    // The live renderer. Only valid while the game is running.
    Renderer& renderer();

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

private:
    friend void run_game(Game&, const std::string&, int, int);
    Renderer* m_renderer = nullptr; // set by run_game for the duration of the run
};

// Open a window and run the game loop until the game stops.
void run_game(Game& game, const std::string& title, int width, int height);

} // namespace loom
