#include "input/input.hpp"

namespace loom {

std::array<bool, Input::KEY_COUNT> Input::s_current  = {};
std::array<bool, Input::KEY_COUNT> Input::s_previous = {};
std::array<bool, 4>                Input::s_mouse_current  = {};
std::array<bool, 4>                Input::s_mouse_previous = {};
Vec2                               Input::s_mouse_pos   = {};
bool                               Input::s_use_injected = false;

void Input::update() {
    if (s_use_injected) return; // test mode: state managed via inject_*

    s_previous       = s_current;
    s_mouse_previous = s_mouse_current;

    int key_count = 0;
    const bool* keys = SDL_GetKeyboardState(&key_count);
    int n = std::min(key_count, KEY_COUNT);
    for (int i = 0; i < n; ++i) s_current[i] = keys[i];

    float mx, my;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    s_mouse_pos = {mx, my};
    s_mouse_current[1] = (buttons & SDL_BUTTON_LMASK)  != 0;
    s_mouse_current[2] = (buttons & SDL_BUTTON_MMASK)  != 0;
    s_mouse_current[3] = (buttons & SDL_BUTTON_RMASK)  != 0;
}

bool Input::key_down(Key k) {
    int i = static_cast<int>(k);
    return i >= 0 && i < KEY_COUNT && s_current[i];
}

bool Input::key_pressed(Key k) {
    int i = static_cast<int>(k);
    return i >= 0 && i < KEY_COUNT && s_current[i] && !s_previous[i];
}

bool Input::key_released(Key k) {
    int i = static_cast<int>(k);
    return i >= 0 && i < KEY_COUNT && !s_current[i] && s_previous[i];
}

Vec2 Input::mouse_position() { return s_mouse_pos; }

void Input::set_mouse_position(Vec2 pos) { s_mouse_pos = pos; }

bool Input::mouse_down(MouseButton btn) {
    int i = static_cast<int>(btn);
    return i >= 1 && i <= 3 && s_mouse_current[i];
}

bool Input::mouse_pressed(MouseButton btn) {
    int i = static_cast<int>(btn);
    return i >= 1 && i <= 3 && s_mouse_current[i] && !s_mouse_previous[i];
}

bool Input::mouse_released(MouseButton btn) {
    int i = static_cast<int>(btn);
    return i >= 1 && i <= 3 && !s_mouse_current[i] && s_mouse_previous[i];
}

// Test helpers ---------------------------------------------------------------

void Input::inject_key_down(Key k) {
    s_use_injected = true;
    s_previous = s_current;
    int i = static_cast<int>(k);
    if (i >= 0 && i < KEY_COUNT) s_current[i] = true;
}

void Input::inject_key_up(Key k) {
    s_use_injected = true;
    s_previous = s_current;
    int i = static_cast<int>(k);
    if (i >= 0 && i < KEY_COUNT) s_current[i] = false;
}

void Input::inject_mouse_position(Vec2 pos) {
    s_use_injected = true;
    s_mouse_pos = pos;
}

} // namespace loom
