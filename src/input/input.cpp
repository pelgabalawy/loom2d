#include "input/input.hpp"
#include <algorithm>
#include <cmath>

namespace loom {

std::array<bool, Input::KEY_COUNT> Input::s_current  = {};
std::array<bool, Input::KEY_COUNT> Input::s_previous = {};
std::array<bool, 4>                Input::s_mouse_current  = {};
std::array<bool, 4>                Input::s_mouse_previous = {};
Vec2                               Input::s_mouse_pos   = {};
Vec2                               Input::s_wheel       = {};
bool                               Input::s_use_injected = false;

std::vector<Input::Pad>            Input::s_pads;
float                              Input::s_deadzone = 0.15f;

std::vector<Input::Finger>         Input::s_fingers;
std::vector<TouchPoint>            Input::s_touch_began;
std::vector<TouchPoint>            Input::s_touch_ended;

std::string                        Input::s_text_input;
bool                               Input::s_text_active = false;
SDL_Window*                        Input::s_window = nullptr;

// ── Frame lifecycle ─────────────────────────────────────────────────────────

void Input::new_frame() {
    // Per-frame accumulators reset before events are pumped. These are cleared
    // even in test/injected mode (harmless — tests inject after new_frame).
    s_wheel = {0.f, 0.f};
    s_text_input.clear();
    s_touch_began.clear();
    s_touch_ended.clear();
}

void Input::process_event(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_MOUSE_WHEEL:
        s_wheel.x += e.wheel.x;
        s_wheel.y += e.wheel.y;
        break;

    case SDL_EVENT_TEXT_INPUT:
        s_text_input += e.text.text;
        break;

    case SDL_EVENT_GAMEPAD_ADDED: {
        SDL_JoystickID id = e.gdevice.which;
        if (pad_slot_for(id) >= 0) break;          // already open
        SDL_Gamepad* gp = SDL_OpenGamepad(id);
        if (!gp) break;
        int slot = free_pad_slot();
        if (slot < 0) { slot = static_cast<int>(s_pads.size()); s_pads.emplace_back(); }
        Pad& p = s_pads[slot];
        p = Pad{};
        p.handle = gp;
        p.id = id;
        p.connected = true;
        break;
    }

    case SDL_EVENT_GAMEPAD_REMOVED: {
        int slot = pad_slot_for(e.gdevice.which);
        if (slot < 0) break;
        if (s_pads[slot].handle) SDL_CloseGamepad(s_pads[slot].handle);
        s_pads[slot] = Pad{};                        // mark disconnected, reusable
        break;
    }

    case SDL_EVENT_FINGER_DOWN: {
        Finger f;
        f.id       = static_cast<int64_t>(e.tfinger.fingerID);
        f.norm     = {e.tfinger.x, e.tfinger.y};
        f.logical  = f.norm;                          // remapped later by run()
        f.pressure = e.tfinger.pressure;
        s_fingers.push_back(f);
        // Store the raw normalized pos; remap_touches() converts it to logical.
        s_touch_began.push_back({f.id, f.norm, f.pressure});
        break;
    }

    case SDL_EVENT_FINGER_MOTION: {
        int64_t id = static_cast<int64_t>(e.tfinger.fingerID);
        for (auto& f : s_fingers) {
            if (f.id == id) {
                f.norm     = {e.tfinger.x, e.tfinger.y};
                f.pressure = e.tfinger.pressure;
                break;
            }
        }
        break;
    }

    case SDL_EVENT_FINGER_UP: {
        int64_t id = static_cast<int64_t>(e.tfinger.fingerID);
        auto it = std::find_if(s_fingers.begin(), s_fingers.end(),
                               [id](const Finger& f){ return f.id == id; });
        if (it != s_fingers.end()) {
            // Store the raw normalized pos; remap_touches() converts it to logical.
            s_touch_ended.push_back({it->id, it->norm, it->pressure});
            s_fingers.erase(it);
        }
        break;
    }

    default: break;
    }
}

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

    // Gamepads: snapshot button edges + read axes for every open pad.
    for (auto& p : s_pads) {
        p.prev = p.cur;
        if (!p.connected || !p.handle) continue;
        for (int b = 0; b < GP_BTN_COUNT; ++b)
            p.cur[b] = SDL_GetGamepadButton(p.handle,
                                            static_cast<SDL_GamepadButton>(b));
        for (int a = 0; a < GP_AXIS_COUNT; ++a) {
            Sint16 raw = SDL_GetGamepadAxis(p.handle,
                                            static_cast<SDL_GamepadAxis>(a));
            p.axes[a] = raw / 32767.f;
        }
    }
}

void Input::set_window(SDL_Window* w) { s_window = w; }

// ── Keyboard ────────────────────────────────────────────────────────────────

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

// ── Mouse ───────────────────────────────────────────────────────────────────

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

Vec2 Input::mouse_wheel() { return s_wheel; }

// ── Gamepad ─────────────────────────────────────────────────────────────────

int Input::pad_slot_for(SDL_JoystickID id) {
    for (size_t i = 0; i < s_pads.size(); ++i)
        if (s_pads[i].connected && s_pads[i].id == id) return static_cast<int>(i);
    return -1;
}

int Input::free_pad_slot() {
    for (size_t i = 0; i < s_pads.size(); ++i)
        if (!s_pads[i].connected) return static_cast<int>(i);
    return -1;
}

int Input::gamepad_count() {
    int n = 0;
    for (auto& p : s_pads) if (p.connected) ++n;
    return n;
}

bool Input::gamepad_connected(int index) {
    return index >= 0 && index < static_cast<int>(s_pads.size())
        && s_pads[index].connected;
}

bool Input::gamepad_down(GamepadButton b, int index) {
    if (!gamepad_connected(index)) return false;
    int i = static_cast<int>(b);
    return i >= 0 && i < GP_BTN_COUNT && s_pads[index].cur[i];
}

bool Input::gamepad_pressed(GamepadButton b, int index) {
    if (!gamepad_connected(index)) return false;
    int i = static_cast<int>(b);
    return i >= 0 && i < GP_BTN_COUNT && s_pads[index].cur[i] && !s_pads[index].prev[i];
}

bool Input::gamepad_released(GamepadButton b, int index) {
    if (!gamepad_connected(index)) return false;
    int i = static_cast<int>(b);
    return i >= 0 && i < GP_BTN_COUNT && !s_pads[index].cur[i] && s_pads[index].prev[i];
}

float Input::gamepad_axis(GamepadAxis a, int index) {
    if (!gamepad_connected(index)) return 0.f;
    int i = static_cast<int>(a);
    if (i < 0 || i >= GP_AXIS_COUNT) return 0.f;
    float v = s_pads[index].axes[i];
    // Triggers are one-sided (0..1) and shouldn't be dead-zoned.
    if (a == GamepadAxis::TriggerLeft || a == GamepadAxis::TriggerRight)
        return std::clamp(v, 0.f, 1.f);
    // Radial dead-zone for sticks, rescaled so motion past the zone starts at 0.
    if (std::fabs(v) < s_deadzone) return 0.f;
    float sign = v < 0.f ? -1.f : 1.f;
    float scaled = (std::fabs(v) - s_deadzone) / (1.f - s_deadzone);
    return sign * std::clamp(scaled, 0.f, 1.f);
}

void  Input::set_gamepad_deadzone(float dz) { s_deadzone = std::clamp(dz, 0.f, 0.99f); }
float Input::gamepad_deadzone() { return s_deadzone; }

bool Input::gamepad_rumble(float low, float high, int duration_ms, int index) {
    if (!gamepad_connected(index) || !s_pads[index].handle) return false;
    auto to_u16 = [](float f){
        return static_cast<Uint16>(std::clamp(f, 0.f, 1.f) * 0xFFFF);
    };
    return SDL_RumbleGamepad(s_pads[index].handle, to_u16(low), to_u16(high),
                             static_cast<Uint32>(std::max(0, duration_ms)));
}

// ── Touch ───────────────────────────────────────────────────────────────────

int Input::touch_count() { return static_cast<int>(s_fingers.size()); }

std::vector<TouchPoint> Input::touches() {
    std::vector<TouchPoint> out;
    out.reserve(s_fingers.size());
    for (auto& f : s_fingers) out.push_back({f.id, f.logical, f.pressure});
    return out;
}

std::vector<TouchPoint> Input::touches_began() { return s_touch_began; }
std::vector<TouchPoint> Input::touches_ended() { return s_touch_ended; }

void Input::remap_touches(const ScaleResult& sr, int draw_w, int draw_h,
                          int point_w, int point_h) {
    auto remap = [&](const Vec2& norm) {
        // normalized (0..1 of window) → window points → logical units.
        float px = norm.x * point_w;
        float py = norm.y * point_h;
        float lx, ly;
        window_point_to_logical(sr, draw_w, draw_h, point_w, point_h, px, py, lx, ly);
        return Vec2{lx, ly};
    };
    for (auto& f : s_fingers) f.logical = remap(f.norm);
    // began/ended snapshots stored the raw normalized pos at capture time; convert
    // them too so callers always see logical coordinates.
    for (auto& t : s_touch_began) t.position = remap(t.position);
    for (auto& t : s_touch_ended) t.position = remap(t.position);
}

// ── Text input ──────────────────────────────────────────────────────────────

void Input::start_text_input() {
    s_text_active = true;
    if (s_window) SDL_StartTextInput(s_window);
}

void Input::stop_text_input() {
    s_text_active = false;
    if (s_window) SDL_StopTextInput(s_window);
}

bool        Input::text_input_active() { return s_text_active; }
std::string Input::text_input()        { return s_text_input; }

// ── Test injection ──────────────────────────────────────────────────────────

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

void Input::inject_mouse_wheel(Vec2 delta) {
    s_use_injected = true;
    s_wheel = delta;
}

void Input::inject_text_input(const std::string& text) {
    s_use_injected = true;
    s_text_input = text;
}

void Input::inject_gamepad_add(int index) {
    s_use_injected = true;
    if (index < 0) return;
    if (index >= static_cast<int>(s_pads.size())) s_pads.resize(index + 1);
    Pad& p = s_pads[index];
    p = Pad{};
    p.connected = true;
    p.id = static_cast<SDL_JoystickID>(1000 + index); // synthetic id
}

void Input::inject_gamepad_remove(int index) {
    s_use_injected = true;
    if (index >= 0 && index < static_cast<int>(s_pads.size())) s_pads[index] = Pad{};
}

void Input::inject_gamepad_button(int index, GamepadButton b, bool down) {
    s_use_injected = true;
    if (index < 0 || index >= static_cast<int>(s_pads.size())) return;
    int i = static_cast<int>(b);
    if (i < 0 || i >= GP_BTN_COUNT) return;
    s_pads[index].prev[i] = s_pads[index].cur[i];
    s_pads[index].cur[i]  = down;
}

void Input::inject_gamepad_axis(int index, GamepadAxis a, float value) {
    s_use_injected = true;
    if (index < 0 || index >= static_cast<int>(s_pads.size())) return;
    int i = static_cast<int>(a);
    if (i < 0 || i >= GP_AXIS_COUNT) return;
    s_pads[index].axes[i] = value;
}

void Input::inject_touch(int64_t id, Vec2 logical_pos, float pressure) {
    s_use_injected = true;
    auto it = std::find_if(s_fingers.begin(), s_fingers.end(),
                           [id](const Finger& f){ return f.id == id; });
    if (it == s_fingers.end()) {
        Finger f; f.id = id; f.logical = logical_pos; f.norm = logical_pos;
        f.pressure = pressure;
        s_fingers.push_back(f);
        s_touch_began.push_back({id, logical_pos, pressure});
    } else {
        it->logical = logical_pos; it->pressure = pressure;
    }
}

void Input::inject_touch_release(int64_t id) {
    s_use_injected = true;
    auto it = std::find_if(s_fingers.begin(), s_fingers.end(),
                           [id](const Finger& f){ return f.id == id; });
    if (it != s_fingers.end()) {
        s_touch_ended.push_back({it->id, it->logical, it->pressure});
        s_fingers.erase(it);
    }
}

} // namespace loom
