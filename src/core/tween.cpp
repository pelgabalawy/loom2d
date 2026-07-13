#include "core/tween.hpp"
#include <algorithm>
#include <cmath>

namespace loom {

namespace {

constexpr float kPi = 3.14159265358979323846f;
// Penner's constants for the overshoot curves.
constexpr float kBack  = 1.70158f;
constexpr float kBack2 = kBack * 1.525f;      // InOutBack overshoots harder
constexpr float kBack3 = kBack + 1.f;

float out_bounce(float t) {
    constexpr float n = 7.5625f, d = 2.75f;
    if (t < 1.f / d)      return n * t * t;
    if (t < 2.f / d)      { t -= 1.5f  / d; return n * t * t + 0.75f;   }
    if (t < 2.5f / d)     { t -= 2.25f / d; return n * t * t + 0.9375f; }
    t -= 2.625f / d;      return n * t * t + 0.984375f;
}

} // namespace

float ease(Ease easing, float t) {
    t = std::clamp(t, 0.f, 1.f);

    switch (easing) {
    case Ease::Linear:    return t;

    case Ease::InQuad:    return t * t;
    case Ease::OutQuad:   return 1.f - (1.f - t) * (1.f - t);
    case Ease::InOutQuad: return t < 0.5f ? 2.f * t * t
                                          : 1.f - std::pow(-2.f * t + 2.f, 2.f) / 2.f;

    case Ease::InCubic:    return t * t * t;
    case Ease::OutCubic:   return 1.f - std::pow(1.f - t, 3.f);
    case Ease::InOutCubic: return t < 0.5f ? 4.f * t * t * t
                                           : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;

    case Ease::InQuart:    return t * t * t * t;
    case Ease::OutQuart:   return 1.f - std::pow(1.f - t, 4.f);
    case Ease::InOutQuart: return t < 0.5f ? 8.f * t * t * t * t
                                           : 1.f - std::pow(-2.f * t + 2.f, 4.f) / 2.f;

    case Ease::InSine:    return 1.f - std::cos(t * kPi / 2.f);
    case Ease::OutSine:   return std::sin(t * kPi / 2.f);
    case Ease::InOutSine: return -(std::cos(kPi * t) - 1.f) / 2.f;

    case Ease::InExpo:  return t <= 0.f ? 0.f : std::pow(2.f, 10.f * t - 10.f);
    case Ease::OutExpo: return t >= 1.f ? 1.f : 1.f - std::pow(2.f, -10.f * t);
    case Ease::InOutExpo:
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        return t < 0.5f ? std::pow(2.f,  20.f * t - 10.f) / 2.f
                        : (2.f - std::pow(2.f, -20.f * t + 10.f)) / 2.f;

    case Ease::InCirc:  return 1.f - std::sqrt(1.f - t * t);
    case Ease::OutCirc: return std::sqrt(1.f - (t - 1.f) * (t - 1.f));
    case Ease::InOutCirc:
        return t < 0.5f
            ? (1.f - std::sqrt(1.f - std::pow(2.f * t, 2.f))) / 2.f
            : (std::sqrt(1.f - std::pow(-2.f * t + 2.f, 2.f)) + 1.f) / 2.f;

    case Ease::InBack:  return kBack3 * t * t * t - kBack * t * t;
    case Ease::OutBack: return 1.f + kBack3 * std::pow(t - 1.f, 3.f)
                                   + kBack  * std::pow(t - 1.f, 2.f);
    case Ease::InOutBack:
        return t < 0.5f
            ? (std::pow(2.f * t, 2.f) * ((kBack2 + 1.f) * 2.f * t - kBack2)) / 2.f
            : (std::pow(2.f * t - 2.f, 2.f)
                  * ((kBack2 + 1.f) * (t * 2.f - 2.f) + kBack2) + 2.f) / 2.f;

    case Ease::InElastic: {
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        constexpr float c = 2.f * kPi / 3.f;
        return -std::pow(2.f, 10.f * t - 10.f) * std::sin((t * 10.f - 10.75f) * c);
    }
    case Ease::OutElastic: {
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        constexpr float c = 2.f * kPi / 3.f;
        return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * c) + 1.f;
    }
    case Ease::InOutElastic: {
        if (t <= 0.f) return 0.f;
        if (t >= 1.f) return 1.f;
        constexpr float c = 2.f * kPi / 4.5f;
        return t < 0.5f
            ? -(std::pow(2.f,  20.f * t - 10.f) * std::sin((20.f * t - 11.125f) * c)) / 2.f
            :  (std::pow(2.f, -20.f * t + 10.f) * std::sin((20.f * t - 11.125f) * c)) / 2.f + 1.f;
    }

    case Ease::InBounce:  return 1.f - out_bounce(1.f - t);
    case Ease::OutBounce: return out_bounce(t);
    case Ease::InOutBounce:
        return t < 0.5f ? (1.f - out_bounce(1.f - 2.f * t)) / 2.f
                        : (1.f + out_bounce(2.f * t - 1.f)) / 2.f;
    }
    return t;
}

// ── Tween ────────────────────────────────────────────────────────────────────

Tween::Tween(float from_, float to_, float duration_, Ease easing_)
    : from(from_), to(to_), duration(duration_), easing(easing_), m_value(from_) {}

void Tween::update(float dt) {
    if (m_done || m_cancelled) return;

    m_elapsed += dt;

    const float t = m_elapsed - delay;
    if (t < 0.f) return;                    // still waiting out the delay

    // A zero-length tween is not a division by zero: it simply arrives at once.
    const float p = duration > 0.f ? std::min(t / duration, 1.f) : 1.f;

    m_value = from + (to - from) * ease(easing, p);
    if (on_update) on_update(m_value);

    if (p >= 1.f) {
        m_done = true;
        if (on_complete) on_complete();
    }
}

void Tween::cancel() { m_cancelled = true; }

float Tween::progress() const {
    if (duration <= 0.f) return m_done ? 1.f : 0.f;
    return std::clamp((m_elapsed - delay) / duration, 0.f, 1.f);
}

// ── TweenManager ─────────────────────────────────────────────────────────────

std::shared_ptr<Tween> TweenManager::add(std::shared_ptr<Tween> tween) {
    if (!tween) return nullptr;
    // Mid-update (an on_complete chaining the next tween): hold it back so it
    // doesn't also run this frame.
    if (m_updating) m_pending.push_back(tween);
    else            m_tweens.push_back(tween);
    return tween;
}

std::shared_ptr<Tween> TweenManager::to(float from, float to, float duration,
                                        Ease easing,
                                        std::function<void(float)> on_update) {
    auto tween = std::make_shared<Tween>(from, to, duration, easing);
    tween->on_update = std::move(on_update);
    return add(std::move(tween));
}

bool TweenManager::cancel(const std::shared_ptr<Tween>& tween) {
    if (!tween) return false;
    for (const auto& list : {&m_tweens, &m_pending}) {
        for (const auto& t : *list) {
            if (t == tween) {
                tween->cancel();   // dropped by the next update()
                return true;
            }
        }
    }
    return false;
}

void TweenManager::clear() {
    for (auto& t : m_tweens)  t->cancel();
    for (auto& t : m_pending) t->cancel();
    if (!m_updating) {
        m_tweens.clear();
        m_pending.clear();
    }
}

std::size_t TweenManager::count() const {
    std::size_t n = 0;
    for (const auto& list : {&m_tweens, &m_pending}) {
        for (const auto& t : *list) {
            if (!t->done() && !t->cancelled()) ++n;
        }
    }
    return n;
}

void TweenManager::update(float dt) {
    m_updating = true;

    const std::size_t n = m_tweens.size();
    for (std::size_t i = 0; i < n; ++i) {
        auto& t = m_tweens[i];
        if (!t->cancelled()) t->update(dt);
    }

    m_updating = false;

    m_tweens.erase(std::remove_if(m_tweens.begin(), m_tweens.end(),
                                  [](const std::shared_ptr<Tween>& t) {
                                      return t->done() || t->cancelled();
                                  }),
                   m_tweens.end());

    for (auto& t : m_pending) {
        if (!t->cancelled()) m_tweens.push_back(std::move(t));
    }
    m_pending.clear();
}

} // namespace loom
