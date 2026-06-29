#pragma once
#include "math/vec2.hpp"
#include "graphics/scaling.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include <vector>

namespace loom {

// Key codes — thin wrapper around SDL_Scancode so callers don't import SDL
enum class Key : int {
    A = SDL_SCANCODE_A, B = SDL_SCANCODE_B, C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D, E = SDL_SCANCODE_E, F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G, H = SDL_SCANCODE_H, I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J, K = SDL_SCANCODE_K, L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M, N = SDL_SCANCODE_N, O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P, Q = SDL_SCANCODE_Q, R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S, T = SDL_SCANCODE_T, U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V, W = SDL_SCANCODE_W, X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y, Z = SDL_SCANCODE_Z,

    Up    = SDL_SCANCODE_UP,    Down  = SDL_SCANCODE_DOWN,
    Left  = SDL_SCANCODE_LEFT,  Right = SDL_SCANCODE_RIGHT,

    Space  = SDL_SCANCODE_SPACE,  Enter  = SDL_SCANCODE_RETURN,
    Escape = SDL_SCANCODE_ESCAPE, Tab    = SDL_SCANCODE_TAB,
    Shift  = SDL_SCANCODE_LSHIFT, Ctrl   = SDL_SCANCODE_LCTRL,
    Alt    = SDL_SCANCODE_LALT,

    // Editing keys — useful alongside text input for building UI text fields
    Backspace = SDL_SCANCODE_BACKSPACE, Delete = SDL_SCANCODE_DELETE,
    Home      = SDL_SCANCODE_HOME,      End    = SDL_SCANCODE_END,

    F1 = SDL_SCANCODE_F1, F2 = SDL_SCANCODE_F2, F3  = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4, F5 = SDL_SCANCODE_F5, F12 = SDL_SCANCODE_F12,

    Num0 = SDL_SCANCODE_0, Num1 = SDL_SCANCODE_1, Num2 = SDL_SCANCODE_2,
    Num3 = SDL_SCANCODE_3, Num4 = SDL_SCANCODE_4, Num5 = SDL_SCANCODE_5,
    Num6 = SDL_SCANCODE_6, Num7 = SDL_SCANCODE_7, Num8 = SDL_SCANCODE_8,
    Num9 = SDL_SCANCODE_9,
};

enum class MouseButton : int { Left = 1, Middle = 2, Right = 3 };

// Gamepad face/shoulder/dpad buttons — mirrors SDL_GamepadButton. "South/East/
// West/North" are layout-neutral (A/B/X/Y on Xbox, ✕/○/□/△ on PlayStation).
enum class GamepadButton : int {
    South         = SDL_GAMEPAD_BUTTON_SOUTH,
    East          = SDL_GAMEPAD_BUTTON_EAST,
    West          = SDL_GAMEPAD_BUTTON_WEST,
    North         = SDL_GAMEPAD_BUTTON_NORTH,
    Back          = SDL_GAMEPAD_BUTTON_BACK,
    Guide         = SDL_GAMEPAD_BUTTON_GUIDE,
    Start         = SDL_GAMEPAD_BUTTON_START,
    LeftStick     = SDL_GAMEPAD_BUTTON_LEFT_STICK,
    RightStick    = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    LeftShoulder  = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    RightShoulder = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    DpadUp        = SDL_GAMEPAD_BUTTON_DPAD_UP,
    DpadDown      = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    DpadLeft      = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    DpadRight     = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
};

// Analog axes — mirrors SDL_GamepadAxis. Sticks report -1..1, triggers 0..1.
enum class GamepadAxis : int {
    LeftX        = SDL_GAMEPAD_AXIS_LEFTX,
    LeftY        = SDL_GAMEPAD_AXIS_LEFTY,
    RightX       = SDL_GAMEPAD_AXIS_RIGHTX,
    RightY       = SDL_GAMEPAD_AXIS_RIGHTY,
    TriggerLeft  = SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
    TriggerRight = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
};

// A single active finger on a touch surface. Position is in logical (design)
// units — run() remaps it through the active scale mode like the mouse pointer.
struct TouchPoint {
    int64_t id       = 0;     // SDL finger id (stable for the touch's lifetime)
    Vec2    position = {};     // logical units
    float   pressure = 1.f;   // 0..1
};

class Input {
public:
    // ── Frame lifecycle (driven by run()) ──────────────────────────────────
    // Call at the very top of the frame, BEFORE polling SDL events: clears the
    // per-frame accumulators (wheel delta, typed text, touch began/ended lists).
    static void new_frame();
    // Feed a single SDL event (wheel, text, gamepad hot-plug, finger events).
    static void process_event(const SDL_Event& e);
    // Snapshot polled device state (keyboard, mouse, gamepad buttons/axes).
    static void update();

    // Window handle used for text-input activation (set once by run()).
    static void set_window(SDL_Window* w);

    // ── Keyboard ────────────────────────────────────────────────────────────
    static bool key_down(Key k);
    static bool key_pressed(Key k);
    static bool key_released(Key k);

    // ── Mouse ───────────────────────────────────────────────────────────────
    // Position is in logical (design) units: run() remaps the raw OS pointer
    // through the active scale mode each frame, so it lines up with the world.
    static Vec2 mouse_position();
    static void set_mouse_position(Vec2 pos);
    static bool mouse_down(MouseButton btn);
    static bool mouse_pressed(MouseButton btn);
    static bool mouse_released(MouseButton btn);
    // Scroll delta accumulated this frame (y > 0 = scrolled up/away).
    static Vec2 mouse_wheel();

    // ── Gamepad ─────────────────────────────────────────────────────────────
    // `index` selects among connected pads (0 = first). Out-of-range / missing
    // pads report not-pressed and zero axes, so callers needn't guard.
    static int   gamepad_count();
    static bool  gamepad_connected(int index = 0);
    static bool  gamepad_down(GamepadButton b, int index = 0);
    static bool  gamepad_pressed(GamepadButton b, int index = 0);
    static bool  gamepad_released(GamepadButton b, int index = 0);
    // Axis value with the radial dead-zone applied (sticks -1..1, triggers 0..1).
    static float gamepad_axis(GamepadAxis a, int index = 0);
    // Dead-zone for stick axes (0..1, default 0.15). Triggers are not dead-zoned.
    static void  set_gamepad_deadzone(float dz);
    static float gamepad_deadzone();
    // Low/high-frequency rumble (0..1) for `duration_ms`. No-op if unsupported.
    static bool  gamepad_rumble(float low, float high, int duration_ms, int index = 0);

    // ── Touch ───────────────────────────────────────────────────────────────
    static int                     touch_count();          // active fingers
    static std::vector<TouchPoint> touches();              // all active
    static std::vector<TouchPoint> touches_began();        // started this frame
    static std::vector<TouchPoint> touches_ended();        // ended this frame
    // Remap stored normalized finger coords into logical units (called by run()).
    static void remap_touches(const ScaleResult& sr, int draw_w, int draw_h,
                              int point_w, int point_h);

    // ── Text input ──────────────────────────────────────────────────────────
    // Enable SDL text events for the window so typed characters arrive; needed
    // for UI text fields. Off by default (so game keys aren't double-handled).
    static void        start_text_input();
    static void        stop_text_input();
    static bool        text_input_active();
    // UTF-8 text typed this frame (empty if none / inactive).
    static std::string text_input();

    // ── Test injection (no real SDL window/devices required) ─────────────────
    static void inject_key_down(Key k);
    static void inject_key_up(Key k);
    static void inject_mouse_position(Vec2 pos);
    static void inject_mouse_wheel(Vec2 delta);
    static void inject_text_input(const std::string& text);
    static void inject_gamepad_add(int index);
    static void inject_gamepad_remove(int index);
    static void inject_gamepad_button(int index, GamepadButton b, bool down);
    static void inject_gamepad_axis(int index, GamepadAxis a, float value);
    static void inject_touch(int64_t id, Vec2 logical_pos, float pressure = 1.f);
    static void inject_touch_release(int64_t id);

private:
    static constexpr int KEY_COUNT     = SDL_SCANCODE_COUNT;
    static constexpr int GP_BTN_COUNT  = SDL_GAMEPAD_BUTTON_COUNT;
    static constexpr int GP_AXIS_COUNT = SDL_GAMEPAD_AXIS_COUNT;

    // Per-physical-gamepad state (one slot per connected/injected pad).
    struct Pad {
        SDL_Gamepad*    handle    = nullptr; // null for injected pads
        SDL_JoystickID  id        = 0;
        bool            connected = false;
        std::array<bool, GP_BTN_COUNT>   cur{};
        std::array<bool, GP_BTN_COUNT>   prev{};
        std::array<float, GP_AXIS_COUNT> axes{};
    };

    // A finger as reported by SDL: raw normalized 0..1 + remapped logical pos.
    struct Finger {
        int64_t id       = 0;
        Vec2    norm     = {};  // raw 0..1 from SDL
        Vec2    logical  = {};  // remapped (run() fills this in)
        float   pressure = 1.f;
    };

    static int pad_slot_for(SDL_JoystickID id);     // -1 if none
    static int free_pad_slot();                     // reuse a disconnected slot

    static std::array<bool, KEY_COUNT> s_current;
    static std::array<bool, KEY_COUNT> s_previous;
    static std::array<bool, 4>         s_mouse_current;
    static std::array<bool, 4>         s_mouse_previous;
    static Vec2                        s_mouse_pos;
    static Vec2                        s_wheel;
    static bool                        s_use_injected;

    static std::vector<Pad>            s_pads;
    static float                       s_deadzone;

    static std::vector<Finger>         s_fingers;       // active
    static std::vector<TouchPoint>     s_touch_began;   // this frame
    static std::vector<TouchPoint>     s_touch_ended;   // this frame

    static std::string                 s_text_input;
    static bool                        s_text_active;
    static SDL_Window*                 s_window;
};

} // namespace loom
