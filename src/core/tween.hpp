#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace loom {

// Easing curves: how a value travels from its start to its end.
//
// In* accelerates away from the start, Out* decelerates into the end, InOut*
// does both. They are the standard Penner curves, so a value tweened with the
// same name here and in any other engine moves identically.
enum class Ease {
    Linear,
    InQuad,    OutQuad,    InOutQuad,
    InCubic,   OutCubic,   InOutCubic,
    InQuart,   OutQuart,   InOutQuart,
    InSine,    OutSine,    InOutSine,
    InExpo,    OutExpo,    InOutExpo,
    InCirc,    OutCirc,    InOutCirc,
    InBack,    OutBack,    InOutBack,
    InElastic, OutElastic, InOutElastic,
    InBounce,  OutBounce,  InOutBounce,
};

// Shape a normalised time (0..1) with an easing curve. t is clamped, so callers
// need not. Most curves return 0 at t=0 and 1 at t=1; Back and Elastic
// deliberately overshoot in between.
float ease(Ease easing, float t);

// One value moving from `from` to `to` over `duration` seconds.
//
// A Tween computes a number and hands it to on_update — it knows nothing about
// what the number is for. That keeps the core free of the scene graph (and of
// Python): the bindings build the "animate this attribute" sugar on top by
// making on_update a setter.
class Tween {
public:
    Tween(float from, float to, float duration, Ease easing = Ease::Linear);

    // Advance the tween, calling on_update() with the new value, then
    // on_complete() on the frame it arrives.
    void update(float dt);

    // Stop early. on_complete does NOT fire — the tween never got there.
    void cancel();

    bool  done()      const { return m_done; }
    bool  cancelled() const { return m_cancelled; }
    // The value right now (`from` until the delay has elapsed).
    float value()     const { return m_value; }
    // How far along, 0..1, ignoring the easing curve.
    float progress()  const;
    float elapsed()   const { return m_elapsed; }

    float from;
    float to;
    float duration;
    Ease  easing;
    // Wait this long before moving. The tween still counts as running.
    float delay = 0.f;

    std::function<void(float)> on_update;
    std::function<void()>      on_complete;

private:
    float m_elapsed   = 0.f;
    float m_value     = 0.f;
    bool  m_done      = false;
    bool  m_cancelled = false;
};

// Runs tweens until they finish, then drops them.
//
// Games hand a tween over and forget it — the manager holds the only reference
// it needs, so `tweens.add(...)` without keeping the return value is fine.
class TweenManager {
public:
    // Take ownership of a tween and start running it next update.
    std::shared_ptr<Tween> add(std::shared_ptr<Tween> tween);
    // Build and run a tween over a bare value.
    std::shared_ptr<Tween> to(float from, float to, float duration, Ease easing,
                              std::function<void(float)> on_update);

    // Cancel a tween and stop running it. Returns false if it wasn't ours.
    bool cancel(const std::shared_ptr<Tween>& tween);
    // Cancel everything in flight.
    void clear();

    // How many tweens are still running.
    std::size_t count() const;

    // Advance every tween, dropping the ones that finished. Driven by the run loop.
    void update(float dt);

private:
    std::vector<std::shared_ptr<Tween>> m_tweens;
    // Tweens started from inside an on_complete callback (chaining) wait here
    // until the current pass is over.
    std::vector<std::shared_ptr<Tween>> m_pending;
    bool                                m_updating = false;
};

} // namespace loom
