#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace loom {

// Identifies a scheduled callback. 0 is never handed out, so it reads as "none".
using TimerHandle = std::uint64_t;

// Run a callback later, or on a repeat.
//
//   timers.after(2.0, spawn_wave)          -- once, in two seconds
//   timers.every(0.5, blink)               -- forever, twice a second
//   timers.every(1.0, tick, times=3)       -- three times, then it retires itself
//
// A callback may schedule and cancel timers freely, including its own handle:
// additions land after the current pass, so a timer that reschedules itself does
// not fire twice in one frame, and a cancelled timer never fires again.
class Timers {
public:
    // Fire fn once, `delay` seconds from now.
    TimerHandle after(float delay, std::function<void()> fn);
    // Fire fn every `interval` seconds. times == 0 repeats forever; otherwise the
    // timer retires after that many fires.
    TimerHandle every(float interval, std::function<void()> fn, int times = 0);

    // Stop a timer. Returns false if the handle was unknown or already retired.
    bool cancel(TimerHandle handle);
    // Stop every timer.
    void clear();

    // True while the handle names a timer that can still fire.
    bool active(TimerHandle handle) const;
    // How many timers are still scheduled.
    std::size_t count() const;

    // Advance every timer. Driven by the run loop.
    void update(float dt);

private:
    struct Timer {
        TimerHandle           id;
        float                 interval;   // 0 for a one-shot
        float                 remaining;  // seconds until the next fire
        int                   repeats;    // fires left; -1 == forever
        bool                  cancelled;
        std::function<void()> fn;
    };

    Timer* find(TimerHandle handle);
    const Timer* find(TimerHandle handle) const;
    TimerHandle add(Timer timer);

    std::vector<Timer> m_timers;
    // Timers scheduled from inside a callback wait here until the pass is over,
    // so update() can hold references into m_timers without it reallocating.
    std::vector<Timer> m_pending;
    bool               m_updating = false;
    TimerHandle        m_next_id  = 1;
};

} // namespace loom
