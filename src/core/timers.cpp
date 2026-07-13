#include "core/timers.hpp"
#include <algorithm>

namespace loom {

// A callback that keeps re-arming a zero-length timer would spin update() for
// ever; cap the catch-up fires per timer per frame instead.
static constexpr int kMaxFiresPerUpdate = 1000;

// Treat a timer as due once it is within this of zero. Float time never lands
// exactly on the mark: an every(0.1) caught up over a one-second frame reaches
// its tenth fire at 1.1e-8 rather than 0, and a strict `<= 0` would skip it and
// then drift a whole interval behind. 100 microseconds is far below a frame and
// far above the rounding error.
static constexpr float kDue = 1e-4f;

TimerHandle Timers::add(Timer timer) {
    timer.id = m_next_id++;
    // Mid-update, park it: appending to m_timers would invalidate update()'s
    // reference into the vector.
    if (m_updating) {
        m_pending.push_back(std::move(timer));
        return m_pending.back().id;
    }
    m_timers.push_back(std::move(timer));
    return m_timers.back().id;
}

TimerHandle Timers::after(float delay, std::function<void()> fn) {
    if (!fn) return 0;
    return add(Timer{0, 0.f, delay, 1, false, std::move(fn)});
}

TimerHandle Timers::every(float interval, std::function<void()> fn, int times) {
    if (!fn) return 0;
    // times <= 0 means "keep going"; the first fire is one interval from now.
    const int repeats = times > 0 ? times : -1;
    return add(Timer{0, interval, interval, repeats, false, std::move(fn)});
}

Timers::Timer* Timers::find(TimerHandle handle) {
    return const_cast<Timer*>(static_cast<const Timers*>(this)->find(handle));
}

const Timers::Timer* Timers::find(TimerHandle handle) const {
    if (handle == 0) return nullptr;
    for (const auto& list : {&m_timers, &m_pending}) {
        for (const Timer& t : *list) {
            if (t.id == handle && !t.cancelled) return &t;
        }
    }
    return nullptr;
}

bool Timers::cancel(TimerHandle handle) {
    Timer* t = find(handle);
    if (!t) return false;
    t->cancelled = true;
    return true;
}

void Timers::clear() {
    // Mid-update the pass is still walking m_timers, so mark rather than erase;
    // the compaction at the end of update() drops them.
    for (Timer& t : m_timers) t.cancelled = true;
    for (Timer& t : m_pending) t.cancelled = true;
    if (!m_updating) {
        m_timers.clear();
        m_pending.clear();
    }
}

bool Timers::active(TimerHandle handle) const { return find(handle) != nullptr; }

std::size_t Timers::count() const {
    std::size_t n = 0;
    for (const auto& list : {&m_timers, &m_pending}) {
        for (const Timer& t : *list) {
            if (!t.cancelled) ++n;
        }
    }
    return n;
}

void Timers::update(float dt) {
    m_updating = true;

    // Only walk the timers that existed when the frame began: anything a callback
    // schedules goes to m_pending and starts counting down next frame.
    const std::size_t n = m_timers.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (m_timers[i].cancelled) continue;
        m_timers[i].remaining -= dt;

        for (int fires = 0; fires < kMaxFiresPerUpdate; ++fires) {
            if (m_timers[i].cancelled || m_timers[i].remaining > kDue) break;

            // Copy the callback: it may cancel this timer (which frees the slot's
            // fn at the end of the pass) or clear() the lot.
            auto fn = m_timers[i].fn;
            fn();

            Timer& t = m_timers[i];
            if (t.cancelled) break;
            if (t.repeats > 0 && --t.repeats == 0) {
                t.cancelled = true;   // retired: it fired its last time
                break;
            }
            if (t.interval <= kDue) {
                // Degenerate repeat (every(0, ...), or an interval shorter than a
                // frame is worth): fire once per frame, not until the cap.
                t.remaining = 0.f;
                break;
            }
            // Carry the overshoot so a long frame doesn't drift the schedule.
            t.remaining += t.interval;
        }
    }

    m_updating = false;

    m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(),
                                  [](const Timer& t) { return t.cancelled; }),
                   m_timers.end());

    for (Timer& t : m_pending) {
        if (!t.cancelled) m_timers.push_back(std::move(t));
    }
    m_pending.clear();
}

} // namespace loom
