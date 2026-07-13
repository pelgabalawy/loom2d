#include <gtest/gtest.h>
#include "core/timers.hpp"

using namespace loom;

TEST(Timers, AfterFiresOnceWhenTheDelayElapses) {
    Timers timers;
    int fired = 0;
    timers.after(1.f, [&] { ++fired; });

    timers.update(0.5f);
    EXPECT_EQ(fired, 0);          // not yet
    timers.update(0.5f);
    EXPECT_EQ(fired, 1);
    timers.update(5.f);
    EXPECT_EQ(fired, 1);          // and never again
    EXPECT_EQ(timers.count(), 0u);
}

TEST(Timers, AfterZeroFiresOnTheNextUpdate) {
    Timers timers;
    int fired = 0;
    timers.after(0.f, [&] { ++fired; });

    EXPECT_EQ(fired, 0);          // scheduling alone does not run it
    timers.update(0.016f);
    EXPECT_EQ(fired, 1);
}

TEST(Timers, EveryRepeatsForever) {
    Timers timers;
    int fired = 0;
    timers.every(0.5f, [&] { ++fired; });

    for (int i = 0; i < 10; ++i) timers.update(0.5f);
    EXPECT_EQ(fired, 10);
    EXPECT_EQ(timers.count(), 1u);
}

TEST(Timers, EveryWithACountRetiresItself) {
    Timers timers;
    int fired = 0;
    timers.every(1.f, [&] { ++fired; }, 3);

    for (int i = 0; i < 10; ++i) timers.update(1.f);
    EXPECT_EQ(fired, 3);
    EXPECT_EQ(timers.count(), 0u);
}

TEST(Timers, RepeatCarriesTheOvershootRatherThanDrifting) {
    Timers timers;
    int fired = 0;
    timers.every(1.f, [&] { ++fired; });

    // 0.6s of overshoot on the first fire must not be thrown away, or a timer
    // driven by uneven frames slowly loses time.
    timers.update(1.6f);
    EXPECT_EQ(fired, 1);
    timers.update(0.4f);          // total 2.0s -> the second fire is due
    EXPECT_EQ(fired, 2);
}

TEST(Timers, ALongFrameCatchesUpOnEveryMissedFire) {
    Timers timers;
    int fired = 0;
    timers.every(0.1f, [&] { ++fired; });

    timers.update(1.f);           // ten intervals in one frame
    EXPECT_EQ(fired, 10);
}

TEST(Timers, EveryZeroFiresOncePerFrameNotForever) {
    Timers timers;
    int fired = 0;
    timers.every(0.f, [&] { ++fired; });   // degenerate: would spin if unguarded

    timers.update(0.016f);
    timers.update(0.016f);
    EXPECT_EQ(fired, 2);
}

TEST(Timers, CancelStopsATimerBeforeItFires) {
    Timers timers;
    int fired = 0;
    TimerHandle h = timers.after(1.f, [&] { ++fired; });
    EXPECT_TRUE(timers.active(h));

    EXPECT_TRUE(timers.cancel(h));
    EXPECT_FALSE(timers.active(h));
    EXPECT_FALSE(timers.cancel(h));        // already gone

    timers.update(2.f);
    EXPECT_EQ(fired, 0);
}

TEST(Timers, CancelIsSafeFromInsideTheCallback) {
    Timers timers;
    int fired = 0;
    TimerHandle h = 0;
    h = timers.every(1.f, [&] {
        ++fired;
        timers.cancel(h);          // a repeat that stops itself on some condition
    });

    timers.update(1.f);
    timers.update(10.f);
    EXPECT_EQ(fired, 1);
    EXPECT_EQ(timers.count(), 0u);
}

TEST(Timers, ACallbackCanScheduleAnotherTimer) {
    Timers timers;
    int first = 0, second = 0;
    timers.after(1.f, [&] {
        ++first;
        timers.after(1.f, [&] { ++second; });
    });

    timers.update(1.f);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 0);         // the new timer starts counting next frame,
                                  // it does not inherit this frame's overshoot
    timers.update(1.f);
    EXPECT_EQ(second, 1);
}

TEST(Timers, ClearFromInsideACallbackStopsTheRest) {
    Timers timers;
    int a = 0, b = 0;
    timers.every(1.f, [&] { ++a; timers.clear(); });
    timers.every(1.f, [&] { ++b; });

    timers.update(1.f);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 0);              // cleared before it got its turn
    EXPECT_EQ(timers.count(), 0u);

    timers.update(1.f);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 0);
}

TEST(Timers, ClearDropsEverything) {
    Timers timers;
    int fired = 0;
    timers.after(1.f, [&] { ++fired; });
    timers.every(1.f, [&] { ++fired; });
    EXPECT_EQ(timers.count(), 2u);

    timers.clear();
    EXPECT_EQ(timers.count(), 0u);
    timers.update(5.f);
    EXPECT_EQ(fired, 0);
}

TEST(Timers, HandlesAreDistinctAndNeverZero) {
    Timers timers;
    TimerHandle a = timers.after(1.f, [] {});
    TimerHandle b = timers.after(1.f, [] {});
    EXPECT_NE(a, 0u);
    EXPECT_NE(b, 0u);
    EXPECT_NE(a, b);

    // An empty callback is not schedulable — nothing to call.
    EXPECT_EQ(timers.after(1.f, nullptr), 0u);
    EXPECT_FALSE(timers.active(0));
}
