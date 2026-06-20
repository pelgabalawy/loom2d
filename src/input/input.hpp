#pragma once
#include "math/vec2.hpp"
#include <SDL3/SDL.h>
#include <array>

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

    F1 = SDL_SCANCODE_F1, F2 = SDL_SCANCODE_F2, F3  = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4, F5 = SDL_SCANCODE_F5, F12 = SDL_SCANCODE_F12,

    Num0 = SDL_SCANCODE_0, Num1 = SDL_SCANCODE_1, Num2 = SDL_SCANCODE_2,
    Num3 = SDL_SCANCODE_3, Num4 = SDL_SCANCODE_4, Num5 = SDL_SCANCODE_5,
    Num6 = SDL_SCANCODE_6, Num7 = SDL_SCANCODE_7, Num8 = SDL_SCANCODE_8,
    Num9 = SDL_SCANCODE_9,
};

enum class MouseButton : int { Left = 1, Middle = 2, Right = 3 };

class Input {
public:
    // Called once per frame by the engine to snapshot SDL key state
    static void update();

    // True while the key is held down
    static bool key_down(Key k);

    // True only on the frame the key was first pressed
    static bool key_pressed(Key k);

    // True only on the frame the key was released
    static bool key_released(Key k);

    // Mouse
    static Vec2 mouse_position();
    static bool mouse_down(MouseButton btn);
    static bool mouse_pressed(MouseButton btn);
    static bool mouse_released(MouseButton btn);

    // For testing: inject state without a real SDL window
    static void inject_key_down(Key k);
    static void inject_key_up(Key k);
    static void inject_mouse_position(Vec2 pos);

private:
    static constexpr int KEY_COUNT = SDL_SCANCODE_COUNT;
    static std::array<bool, KEY_COUNT> s_current;
    static std::array<bool, KEY_COUNT> s_previous;
    static std::array<bool, 4>        s_mouse_current;
    static std::array<bool, 4>        s_mouse_previous;
    static Vec2                        s_mouse_pos;
    static bool                        s_use_injected;
};

} // namespace loom
