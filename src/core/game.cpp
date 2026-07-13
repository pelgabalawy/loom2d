#include "core/game.hpp"
#include "platform/window.hpp"
#include "graphics/renderer.hpp"
#include "input/input.hpp"
#include "math/vec2.hpp"
#include <algorithm>
#include <cmath>

namespace loom {

Game::Game() {
    // Let scenes reach back to their game (assets, audio, switching scenes).
    scenes.set_game(this);
}

void run_game(Game& game, const std::string& title, int width, int height) {
    Window   window(title, width, height);
    Renderer renderer(window); // sets up sokol_gfx

    // Resolve the logical/design resolution: default to the initial window size.
    int logical_w = game.logical_width  > 0 ? game.logical_width  : width;
    int logical_h = game.logical_height > 0 ? game.logical_height : height;
    game.logical_width  = logical_w;
    game.logical_height = logical_h;

    // Lay every scene out against the logical screen: world (0,0) at the top-left,
    // matching pixel/screen coordinates. Scenes entering later get the same setup.
    game.scenes.set_screen(logical_w, logical_h);

    // The UI layer is laid out against the logical resolution in screen space,
    // independent of where the world camera roams.
    game.ui.set_screen(logical_w, logical_h);

    // Let Input activate text input against this window when asked.
    Input::set_window(window.sdl_window());

    game.on_start();

    Uint64 last_ticks = SDL_GetTicks();
    int    last_dw = -1, last_dh = -1;

    while (game.running) {
        // Reset per-frame input accumulators, then pump SDL events (which Window
        // forwards into Input). poll_events() returns false on quit/Escape.
        Input::new_frame();
        if (!window.poll_events()) break;
        Input::update();

        Uint64 now = SDL_GetTicks();
        float  dt  = static_cast<float>(now - last_ticks) / 1000.f;
        last_ticks = now;
        dt = std::min(dt, 0.1f);

        // Track the real drawable size (changes on resize / DPI moves) and map
        // the logical resolution onto it according to the chosen scale mode.
        int dw = window.drawable_width();
        int dh = window.drawable_height();
        if (dw != last_dw || dh != last_dh) {
            game.screen_width  = dw;
            game.screen_height = dh;
            if (last_dw >= 0) game.on_resize(dw, dh); // skip the initial frame
            last_dw = dw; last_dh = dh;
        }
        ScaleResult sr = compute_scaling(game.scale_mode, logical_w, logical_h, dw, dh);
        game.scenes.set_camera_viewport(static_cast<int>(std::lround(sr.cam_w)),
                                        static_cast<int>(std::lround(sr.cam_h)));

        // Remap the OS pointer (window points) into logical units so that
        // screen_to_world(mouse_position()) is correct under any scale mode / DPI.
        int pw = dw, ph = dh;
        SDL_GetWindowSize(window.sdl_window(), &pw, &ph);
        Vec2 mp = Input::mouse_position();
        float mlx, mly;
        window_point_to_logical(sr, dw, dh, pw, ph, mp.x, mp.y, mlx, mly);
        Input::set_mouse_position(Vec2(mlx, mly));
        // Remap touch finger positions into logical units the same way.
        Input::remap_touches(sr, dw, dh, pw, ph);

        game.on_update(dt);

        // Lay out both UI layers, then dispatch the pointer using the logical-space
        // mouse from above.
        const Vec2 pointer  = Input::mouse_position();
        const bool pressed  = Input::mouse_pressed(MouseButton::Left);
        const bool down     = Input::mouse_down(MouseButton::Left);
        const bool released = Input::mouse_released(MouseButton::Left);

        game.ui.layout();
        std::shared_ptr<Scene> active = game.scenes.current();
        if (active) active->ui.layout();

        // The game's UI sits above the scene's, so it gets first refusal on the
        // pointer: a click that lands on a global widget must not also fall
        // through to a scene widget underneath it. Input is likewise withheld
        // from a scene that is mid-transition — it is on its way out, and a
        // stray click on a fading menu would fire the wrong thing.
        const bool over_game_ui = game.ui.root().hit_test(pointer) != nullptr;
        game.ui.update_input(pointer, pressed, down, released);

        if (active) {
            if (over_game_ui || game.scenes.transitioning()) {
                // Park the pointer far offscreen: clears hover/press rather than
                // leaving a widget stuck in its highlighted state.
                active->ui.update_input(Vec2(-1e9f, -1e9f), false, false, false);
            } else {
                active->ui.update_input(pointer, pressed, down, released);
            }
        }

        // Timers and tweens run before physics and scenes, so a callback that
        // moves something this frame has it simulated and drawn the same frame.
        if (game.auto_timers)  game.timers.update(dt);
        if (game.auto_tweens)  game.tweens.update(dt);
        if (game.auto_physics) game.physics.step(dt);
        if (game.auto_scene)   game.scenes.update(dt);

        renderer.begin_frame(game.clear_color);
        renderer.set_viewport(sr.vp_x, sr.vp_y, sr.vp_w, sr.vp_h);
        if (game.auto_scene) game.scenes.draw(renderer);
        game.on_draw();
        // Draw the game's UI on top with its own fixed screen-space view-projection.
        // The batcher captures the matrix per-batch, so the single end_frame flush
        // renders the world and the UI in one buffer upload.
        renderer.batcher().set_view_projection(game.ui.camera.view_projection());
        game.ui.draw(renderer);
        // A transition covers everything, including the UI.
        game.scenes.draw_transition(renderer);
        renderer.end_frame();

        game.last_draw_calls = renderer.batcher().draw_calls();
    }

    game.on_stop();
}

} // namespace loom
