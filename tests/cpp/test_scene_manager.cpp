#include <gtest/gtest.h>
#include "scene/scene_manager.hpp"
#include "scene/transition.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace loom;

namespace {

// A scene that records its lifecycle so the ordering can be asserted.
class TracedScene : public Scene {
public:
    TracedScene(std::string name, std::vector<std::string>* log)
        : name(std::move(name)), log(log) {}

    void on_enter()  override { log->push_back(name + ":enter"); }
    void on_exit()   override { log->push_back(name + ":exit");  }
    void on_update(float) override { updates++; }

    std::string               name;
    std::vector<std::string>* log;
    int                       updates = 0;
};

std::shared_ptr<TracedScene> traced(const char* name, std::vector<std::string>* log) {
    return std::make_shared<TracedScene>(name, log);
}

} // namespace

// ── Fade ─────────────────────────────────────────────────────────────────────

TEST(Fade, AlphaPeaksAtMidpointAndReturnsToZero) {
    Fade fade(1.0f);
    EXPECT_FLOAT_EQ(fade.alpha(), 0.f);

    fade.update(0.25f);
    EXPECT_FLOAT_EQ(fade.alpha(), 0.5f);   // half way out

    fade.update(0.25f);
    EXPECT_FLOAT_EQ(fade.alpha(), 1.f);    // fully covered at the midpoint

    fade.update(0.25f);
    EXPECT_FLOAT_EQ(fade.alpha(), 0.5f);   // fading back in

    fade.update(0.25f);
    EXPECT_FLOAT_EQ(fade.alpha(), 0.f);
}

TEST(Fade, SwapReadyAtMidpointDoneAtEnd) {
    Fade fade(1.0f);
    EXPECT_FALSE(fade.swap_ready());
    EXPECT_FALSE(fade.done());

    fade.update(0.5f);
    EXPECT_TRUE(fade.swap_ready());
    EXPECT_FALSE(fade.done());

    fade.update(0.5f);
    EXPECT_TRUE(fade.done());
}

TEST(Fade, ZeroDurationIsACut) {
    Fade fade(0.f);
    EXPECT_TRUE(fade.swap_ready());
    EXPECT_TRUE(fade.done());
    EXPECT_FLOAT_EQ(fade.alpha(), 0.f);  // nothing to draw
}

// ── SceneManager ─────────────────────────────────────────────────────────────

TEST(SceneManager, StartsWithOneScene) {
    SceneManager m;
    EXPECT_EQ(m.depth(), 1u);
    EXPECT_NE(m.current(), nullptr);
}

TEST(SceneManager, SwitchToReplacesAndFiresLifecycle) {
    std::vector<std::string> log;
    SceneManager m;

    auto a = traced("a", &log);
    m.switch_to(a);
    EXPECT_EQ(m.depth(), 1u);          // replaced the default scene, not stacked
    EXPECT_EQ(m.current(), a);
    EXPECT_EQ(log, (std::vector<std::string>{"a:enter"}));

    auto b = traced("b", &log);
    m.switch_to(b);
    EXPECT_EQ(m.current(), b);
    EXPECT_EQ(log, (std::vector<std::string>{"a:enter", "a:exit", "b:enter"}));
}

TEST(SceneManager, PushKeepsSceneBelowAliveAndPaused) {
    std::vector<std::string> log;
    SceneManager m;

    auto level = traced("level", &log);
    m.switch_to(level);
    auto pause = traced("pause", &log);
    m.push(pause);

    EXPECT_EQ(m.depth(), 2u);
    EXPECT_EQ(m.current(), pause);

    m.update(0.016f);
    EXPECT_EQ(pause->updates, 1);
    EXPECT_EQ(level->updates, 0) << "a scene under a pushed one must not update";

    // Popping reveals it again, exactly as it was — no second on_enter.
    m.pop();
    EXPECT_EQ(m.depth(), 1u);
    EXPECT_EQ(m.current(), level);
    EXPECT_EQ(log, (std::vector<std::string>{"level:enter", "pause:enter", "pause:exit"}));

    m.update(0.016f);
    EXPECT_EQ(level->updates, 1) << "the revealed scene resumes updating";
}

TEST(SceneManager, PopNeverEmptiesTheStack) {
    SceneManager m;
    m.pop();
    EXPECT_EQ(m.depth(), 1u);
    EXPECT_NE(m.current(), nullptr) << "current() must stay valid: game.scene relies on it";
}

TEST(SceneManager, TransitionDefersTheSwapToTheMidpoint) {
    std::vector<std::string> log;
    SceneManager m;

    auto from = traced("from", &log);
    m.switch_to(from);
    log.clear();

    auto to = traced("to", &log);
    m.switch_to(to, std::make_shared<Fade>(1.0f));

    // The old scene is still the live one while the screen fades out.
    EXPECT_TRUE(m.transitioning());
    EXPECT_EQ(m.current(), from);
    EXPECT_TRUE(log.empty()) << "nothing should have entered or exited yet";

    m.update(0.25f);
    EXPECT_EQ(m.current(), from) << "swap must not happen before the fade covers the screen";

    m.update(0.25f);   // now at the midpoint
    EXPECT_EQ(m.current(), to);
    EXPECT_EQ(log, (std::vector<std::string>{"from:exit", "to:enter"}));
    EXPECT_TRUE(m.transitioning()) << "still fading back in";

    m.update(0.5f);
    EXPECT_FALSE(m.transitioning());
}

TEST(SceneManager, SceneKeepsUpdatingWhileFadingOut) {
    std::vector<std::string> log;
    SceneManager m;
    auto from = traced("from", &log);
    m.switch_to(from);

    m.switch_to(traced("to", &log), std::make_shared<Fade>(1.0f));
    m.update(0.25f);   // pre-midpoint: `from` is still on top

    EXPECT_EQ(from->updates, 1) << "the outgoing scene should keep animating as it fades";
}

TEST(SceneManager, SecondSwitchMidTransitionSettlesTheFirst) {
    std::vector<std::string> log;
    SceneManager m;
    m.switch_to(traced("a", &log));
    log.clear();

    auto b = traced("b", &log);
    auto c = traced("c", &log);
    m.switch_to(b, std::make_shared<Fade>(1.0f));
    m.switch_to(c, std::make_shared<Fade>(1.0f));   // interrupts before b landed

    // b's swap is applied immediately rather than being dropped, so the stack is
    // never left half-swapped, and c then transitions in from b.
    EXPECT_EQ(m.depth(), 1u);
    EXPECT_EQ(m.current(), b);

    m.update(0.5f);
    EXPECT_EQ(m.current(), c);
    EXPECT_EQ(log, (std::vector<std::string>{"a:exit", "b:enter", "b:exit", "c:enter"}));
}

TEST(SceneManager, EnterConfiguresCameraAgainstLogicalScreen) {
    SceneManager m;
    m.set_screen(320, 240);

    auto s = std::make_shared<Scene>();
    m.switch_to(s);

    // World (0,0) at the top-left of the screen, matching the run loop's default.
    EXPECT_EQ(s->camera.position().x, 160.f);
    EXPECT_EQ(s->camera.position().y, 120.f);
    EXPECT_EQ(s->ui.screen_width(),  320);
    EXPECT_EQ(s->ui.screen_height(), 240);
}
