#include <gtest/gtest.h>
#include "core/tween.hpp"
#include <vector>

using namespace loom;

namespace {

const std::vector<Ease> kAllEasings = {
    Ease::Linear,
    Ease::InQuad,    Ease::OutQuad,    Ease::InOutQuad,
    Ease::InCubic,   Ease::OutCubic,   Ease::InOutCubic,
    Ease::InQuart,   Ease::OutQuart,   Ease::InOutQuart,
    Ease::InSine,    Ease::OutSine,    Ease::InOutSine,
    Ease::InExpo,    Ease::OutExpo,    Ease::InOutExpo,
    Ease::InCirc,    Ease::OutCirc,    Ease::InOutCirc,
    Ease::InBack,    Ease::OutBack,    Ease::InOutBack,
    Ease::InElastic, Ease::OutElastic, Ease::InOutElastic,
    Ease::InBounce,  Ease::OutBounce,  Ease::InOutBounce,
};

} // namespace

// ── Easing curves ────────────────────────────────────────────────────────────

TEST(Ease, EveryCurveStartsAtZeroAndEndsAtOne) {
    // The one property every curve must have: a tween that runs to completion
    // lands exactly on its target, whatever shape it took getting there.
    for (Ease e : kAllEasings) {
        EXPECT_NEAR(ease(e, 0.f), 0.f, 1e-5f) << "at t=0, easing " << static_cast<int>(e);
        EXPECT_NEAR(ease(e, 1.f), 1.f, 1e-5f) << "at t=1, easing " << static_cast<int>(e);
    }
}

TEST(Ease, TimeIsClampedSoOutOfRangeInputIsHarmless) {
    for (Ease e : kAllEasings) {
        EXPECT_NEAR(ease(e, -5.f), 0.f, 1e-5f) << "easing " << static_cast<int>(e);
        EXPECT_NEAR(ease(e,  5.f), 1.f, 1e-5f) << "easing " << static_cast<int>(e);
    }
}

TEST(Ease, LinearIsTheIdentity) {
    EXPECT_NEAR(ease(Ease::Linear, 0.25f), 0.25f, 1e-5f);
    EXPECT_NEAR(ease(Ease::Linear, 0.75f), 0.75f, 1e-5f);
}

TEST(Ease, InCurvesStartSlowAndOutCurvesStartFast) {
    // Half way through, an ease-in has covered less than half the distance and
    // an ease-out more — that is the whole point of the curve.
    EXPECT_LT(ease(Ease::InQuad,   0.5f), 0.5f);
    EXPECT_LT(ease(Ease::InCubic,  0.5f), 0.5f);
    EXPECT_GT(ease(Ease::OutQuad,  0.5f), 0.5f);
    EXPECT_GT(ease(Ease::OutCubic, 0.5f), 0.5f);
    EXPECT_NEAR(ease(Ease::InOutQuad, 0.5f), 0.5f, 1e-5f);   // symmetric
}

TEST(Ease, BackOvershootsPastItsEndpoints) {
    // Back anticipates (dips below 0) then overshoots (past 1); that overshoot is
    // the feature, so guard it against a "clamp everything" regression.
    EXPECT_LT(ease(Ease::InBack,  0.2f), 0.f);
    EXPECT_GT(ease(Ease::OutBack, 0.8f), 1.f);
}

TEST(Ease, OutBounceOnlyApproachesFromBelow) {
    for (float t = 0.f; t <= 1.f; t += 0.05f) {
        const float v = ease(Ease::OutBounce, t);
        EXPECT_GE(v, 0.f);
        EXPECT_LE(v, 1.f + 1e-5f);   // a bounce settles, it never overshoots
    }
}

// ── Tween ────────────────────────────────────────────────────────────────────

TEST(Tween, MovesFromStartToEndOverItsDuration) {
    Tween t(0.f, 100.f, 1.f);
    EXPECT_NEAR(t.value(), 0.f, 1e-5f);

    t.update(0.5f);
    EXPECT_NEAR(t.value(), 50.f, 1e-4f);
    EXPECT_NEAR(t.progress(), 0.5f, 1e-5f);
    EXPECT_FALSE(t.done());

    t.update(0.5f);
    EXPECT_NEAR(t.value(), 100.f, 1e-4f);
    EXPECT_TRUE(t.done());
}

TEST(Tween, OvershootingTheDurationStillLandsExactlyOnTheTarget) {
    Tween t(0.f, 10.f, 1.f, Ease::OutBounce);
    t.update(999.f);
    EXPECT_NEAR(t.value(), 10.f, 1e-4f);
    EXPECT_TRUE(t.done());
    EXPECT_NEAR(t.progress(), 1.f, 1e-5f);
}

TEST(Tween, CallsOnUpdateWithEachValueAndOnCompleteOnce) {
    Tween t(0.f, 1.f, 1.f);
    std::vector<float> values;
    int completed = 0;
    t.on_update   = [&](float v) { values.push_back(v); };
    t.on_complete = [&] { ++completed; };

    for (int i = 0; i < 5; ++i) t.update(0.25f);   // one frame past the end

    EXPECT_EQ(values.size(), 4u);                  // no call after it finished
    EXPECT_NEAR(values.back(), 1.f, 1e-5f);
    EXPECT_EQ(completed, 1);
}

TEST(Tween, DelayHoldsTheValueBeforeItStarts) {
    Tween t(5.f, 10.f, 1.f);
    t.delay = 1.f;
    int updates = 0;
    t.on_update = [&](float) { ++updates; };

    t.update(0.5f);
    EXPECT_NEAR(t.value(), 5.f, 1e-5f);   // still sitting at the start
    EXPECT_EQ(updates, 0);
    EXPECT_NEAR(t.progress(), 0.f, 1e-5f);

    t.update(1.f);                        // 0.5s past the delay
    EXPECT_NEAR(t.value(), 7.5f, 1e-4f);
    EXPECT_EQ(updates, 1);
}

TEST(Tween, ZeroDurationArrivesImmediately) {
    Tween t(0.f, 42.f, 0.f);
    t.update(0.016f);
    EXPECT_NEAR(t.value(), 42.f, 1e-5f);
    EXPECT_TRUE(t.done());
}

TEST(Tween, CancelStopsItWithoutCompleting) {
    Tween t(0.f, 100.f, 1.f);
    int completed = 0;
    t.on_complete = [&] { ++completed; };

    t.update(0.5f);
    t.cancel();
    t.update(0.5f);

    EXPECT_TRUE(t.cancelled());
    EXPECT_FALSE(t.done());
    EXPECT_NEAR(t.value(), 50.f, 1e-4f);   // frozen where it was
    EXPECT_EQ(completed, 0);               // it never arrived, so it never completed
}

// ── TweenManager ─────────────────────────────────────────────────────────────

TEST(TweenManager, RunsTweensAndDropsThemWhenTheyFinish) {
    TweenManager tweens;
    float x = 0.f;
    tweens.to(0.f, 10.f, 1.f, Ease::Linear, [&](float v) { x = v; });
    EXPECT_EQ(tweens.count(), 1u);

    tweens.update(0.5f);
    EXPECT_NEAR(x, 5.f, 1e-4f);
    EXPECT_EQ(tweens.count(), 1u);

    tweens.update(0.5f);
    EXPECT_NEAR(x, 10.f, 1e-4f);
    EXPECT_EQ(tweens.count(), 0u);   // finished tweens do not accumulate
}

TEST(TweenManager, RunsTweensItWasHandedWithoutTheCallerKeepingOne) {
    // The whole point of the manager: fire and forget.
    TweenManager tweens;
    float x = 0.f;
    tweens.add(std::make_shared<Tween>(0.f, 4.f, 1.f))->on_update =
        [&](float v) { x = v; };

    tweens.update(1.f);
    EXPECT_NEAR(x, 4.f, 1e-4f);
}

TEST(TweenManager, OnCompleteCanChainTheNextTween) {
    TweenManager tweens;
    float x = 0.f;
    int  second_updates = 0;

    auto first = std::make_shared<Tween>(0.f, 1.f, 1.f);
    first->on_update   = [&](float v) { x = v; };
    first->on_complete = [&] {
        auto next = std::make_shared<Tween>(1.f, 2.f, 1.f);
        next->on_update = [&](float v) { x = v; ++second_updates; };
        tweens.add(next);
    };
    tweens.add(first);

    tweens.update(1.f);                 // first completes and chains
    EXPECT_NEAR(x, 1.f, 1e-4f);
    EXPECT_EQ(second_updates, 0);       // the chained tween starts next frame
    EXPECT_EQ(tweens.count(), 1u);

    tweens.update(1.f);
    EXPECT_NEAR(x, 2.f, 1e-4f);
    EXPECT_EQ(second_updates, 1);
    EXPECT_EQ(tweens.count(), 0u);
}

TEST(TweenManager, CancelStopsATweenMidFlight) {
    TweenManager tweens;
    float x = 0.f;
    auto t = tweens.to(0.f, 10.f, 1.f, Ease::Linear, [&](float v) { x = v; });

    tweens.update(0.5f);
    EXPECT_TRUE(tweens.cancel(t));
    tweens.update(0.5f);

    EXPECT_NEAR(x, 5.f, 1e-4f);      // never reached 10
    EXPECT_EQ(tweens.count(), 0u);
    EXPECT_FALSE(tweens.cancel(t));  // no longer ours
}

TEST(TweenManager, ClearCancelsEverythingInFlight) {
    TweenManager tweens;
    float x = 0.f, y = 0.f;
    tweens.to(0.f, 10.f, 1.f, Ease::Linear, [&](float v) { x = v; });
    tweens.to(0.f, 10.f, 1.f, Ease::Linear, [&](float v) { y = v; });
    EXPECT_EQ(tweens.count(), 2u);

    tweens.clear();
    EXPECT_EQ(tweens.count(), 0u);

    tweens.update(1.f);
    EXPECT_NEAR(x, 0.f, 1e-5f);
    EXPECT_NEAR(y, 0.f, 1e-5f);
}
