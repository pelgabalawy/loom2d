#include <gtest/gtest.h>
#include "ui/widget.hpp"
#include "ui/widgets.hpp"
#include "ui/ui_canvas.hpp"

#include <memory>

using namespace loom;

// All UI layout, hit-testing and pointer dispatch is GPU-free, so it is fully
// unit-testable without a window. Only draw() (not exercised here) needs a GL
// context.

// ── compute_anchored_rect ───────────────────────────────────────────────────

TEST(AnchoredRect, TopLeftDefault) {
    Rect r = compute_anchored_rect({0, 0, 800, 600}, {0, 0}, {0, 0}, {0, 0}, {100, 40});
    EXPECT_FLOAT_EQ(r.x, 0.f);
    EXPECT_FLOAT_EQ(r.y, 0.f);
    EXPECT_FLOAT_EQ(r.w, 100.f);
    EXPECT_FLOAT_EQ(r.h, 40.f);
}

TEST(AnchoredRect, Centered) {
    Rect r = compute_anchored_rect({0, 0, 800, 600}, {0.5f, 0.5f}, {0.5f, 0.5f},
                                   {0, 0}, {100, 40});
    EXPECT_FLOAT_EQ(r.x, 350.f);
    EXPECT_FLOAT_EQ(r.y, 280.f);
}

TEST(AnchoredRect, BottomRightWithInsetOffset) {
    // anchor & pivot bottom-right; offset acts as an inset toward top-left.
    Rect r = compute_anchored_rect({0, 0, 800, 600}, {1.f, 1.f}, {1.f, 1.f},
                                   {-10, -10}, {100, 40});
    EXPECT_FLOAT_EQ(r.right(),  790.f);
    EXPECT_FLOAT_EQ(r.bottom(), 590.f);
}

TEST(AnchoredRect, ZeroSizeFillsParent) {
    Rect r = compute_anchored_rect({5, 7, 800, 600}, {0, 0}, {0, 0}, {0, 0}, {0, 0});
    EXPECT_FLOAT_EQ(r.x, 5.f);
    EXPECT_FLOAT_EQ(r.y, 7.f);
    EXPECT_FLOAT_EQ(r.w, 800.f);
    EXPECT_FLOAT_EQ(r.h, 600.f);
}

TEST(AnchoredRect, NegativeSizeInsetsParent) {
    Rect r = compute_anchored_rect({0, 0, 800, 600}, {0, 0}, {0, 0}, {0, 0}, {-40, -20});
    EXPECT_FLOAT_EQ(r.w, 760.f);
    EXPECT_FLOAT_EQ(r.h, 580.f);
}

// ── Widget tree & layout ────────────────────────────────────────────────────

TEST(Widget, AddChildSetsParentAndRecursesLayout) {
    auto parent = std::make_shared<Panel>();
    parent->size = {0, 0}; // fill
    auto child = std::make_shared<Panel>();
    child->anchor = {0.5f, 0.5f}; child->pivot = {0.5f, 0.5f}; child->size = {50, 50};
    parent->add_child(child);
    EXPECT_EQ(child->parent(), parent.get());

    parent->resolve_layout({0, 0, 200, 200});
    EXPECT_FLOAT_EQ(parent->rect().w, 200.f);
    EXPECT_FLOAT_EQ(child->rect().x, 75.f); // centred 50 in 200
    EXPECT_FLOAT_EQ(child->rect().y, 75.f);
}

TEST(Widget, RemoveFromParent) {
    auto parent = std::make_shared<Widget>();
    auto child  = std::make_shared<Widget>();
    parent->add_child(child);
    ASSERT_EQ(parent->children().size(), 1u);
    child->remove_from_parent();
    EXPECT_TRUE(parent->children().empty());
    EXPECT_EQ(child->parent(), nullptr);
}

// ── Hit testing ─────────────────────────────────────────────────────────────

TEST(HitTest, TopmostChildWins) {
    auto root = std::make_shared<Widget>();
    auto a = std::make_shared<Panel>();  a->size = {0, 0}; // fills
    auto b = std::make_shared<Panel>();  b->size = {0, 0}; // fills, added later (on top)
    root->add_child(a);
    root->add_child(b);
    root->place({0, 0, 100, 100});
    EXPECT_EQ(root->hit_test({50, 50}), b.get());
}

TEST(HitTest, SkipsDisabledAndInvisible) {
    auto root = std::make_shared<Widget>();
    auto a = std::make_shared<Panel>(); a->size = {0, 0};
    root->add_child(a);
    root->place({0, 0, 100, 100});

    a->enabled = false;
    EXPECT_EQ(root->hit_test({50, 50}), root.get()); // falls through to root
    a->enabled = true; a->visible = false;
    EXPECT_EQ(root->hit_test({50, 50}), root.get());
}

TEST(HitTest, MissReturnsNull) {
    auto root = std::make_shared<Widget>();
    root->place({0, 0, 100, 100});
    EXPECT_EQ(root->hit_test({200, 200}), nullptr);
}

TEST(Widget, IsInSubtree) {
    auto root  = std::make_shared<Widget>();
    auto child = std::make_shared<Widget>();
    root->add_child(child);
    Widget stranger;
    EXPECT_TRUE(root->is_in_subtree(child.get()));
    EXPECT_TRUE(root->is_in_subtree(root.get()));
    EXPECT_FALSE(root->is_in_subtree(&stranger));
}

// ── UICanvas pointer dispatch ───────────────────────────────────────────────

namespace {
std::shared_ptr<Button> centred_button(UICanvas& c, int counter[1]) {
    auto b = std::make_shared<Button>();
    b->anchor = {0.5f, 0.5f}; b->pivot = {0.5f, 0.5f}; b->size = {100, 40};
    b->on_clicked = [counter]{ counter[0]++; };
    c.add(b);
    return b;
}
} // namespace

TEST(UICanvas, ClickFiresOnPressThenReleaseOverSameWidget) {
    UICanvas c; c.set_screen(800, 600);
    int clicks[1] = {0};
    auto b = centred_button(c, clicks);
    c.layout();

    Vec2 centre{400, 300};
    c.update_input(centre, /*pressed*/true,  /*down*/true,  /*released*/false);
    EXPECT_TRUE(b->hovered);
    EXPECT_TRUE(b->pressed);
    EXPECT_TRUE(b->focused);
    c.update_input(centre, false, false, true);
    EXPECT_EQ(clicks[0], 1);
    EXPECT_FALSE(b->pressed);
}

TEST(UICanvas, NoClickWhenReleasedOutside) {
    UICanvas c; c.set_screen(800, 600);
    int clicks[1] = {0};
    auto b = centred_button(c, clicks);
    c.layout();

    c.update_input({400, 300}, true, true, false);
    c.update_input({10, 10},   false, false, true); // released off the button
    EXPECT_EQ(clicks[0], 0);
}

TEST(UICanvas, PressedOnlyWhilePointerStaysOverTarget) {
    UICanvas c; c.set_screen(800, 600);
    int clicks[1] = {0};
    auto b = centred_button(c, clicks);
    c.layout();

    c.update_input({400, 300}, true, true, false);
    EXPECT_TRUE(b->pressed);
    c.update_input({10, 10}, false, true, false); // dragged off, still held
    EXPECT_FALSE(b->pressed);
    c.update_input({400, 300}, false, true, false); // dragged back
    EXPECT_TRUE(b->pressed);
}

TEST(UICanvas, HoverTransfersBetweenWidgets) {
    UICanvas c; c.set_screen(800, 600);
    auto a = std::make_shared<Button>();
    a->anchor = {0, 0}; a->pivot = {0, 0}; a->offset = {0, 0};   a->size = {100, 40};
    auto b = std::make_shared<Button>();
    b->anchor = {0, 0}; b->pivot = {0, 0}; b->offset = {200, 0}; b->size = {100, 40};
    c.add(a); c.add(b);
    c.layout();

    c.update_input({50, 20}, false, false, false);
    EXPECT_TRUE(a->hovered);  EXPECT_FALSE(b->hovered);
    c.update_input({250, 20}, false, false, false);
    EXPECT_FALSE(a->hovered); EXPECT_TRUE(b->hovered);
}

TEST(UICanvas, ClickEmptySpaceClearsFocus) {
    UICanvas c; c.set_screen(800, 600);
    int clicks[1] = {0};
    auto b = centred_button(c, clicks);
    c.layout();
    c.update_input({400, 300}, true, true, false);
    EXPECT_TRUE(b->focused);
    // press in empty space (corner not covered by the button)
    c.update_input({5, 5}, true, true, false);
    EXPECT_FALSE(b->focused);
}

TEST(UICanvas, RemovingFocusedWidgetClearsCachedPointers) {
    UICanvas c; c.set_screen(800, 600);
    int clicks[1] = {0};
    auto b = centred_button(c, clicks);
    c.layout();
    c.update_input({400, 300}, true, true, false);
    ASSERT_TRUE(b->focused);
    c.remove(b.get());
    EXPECT_EQ(c.focused(), nullptr);
    // Next dispatch must not touch the removed widget.
    c.layout();
    c.update_input({400, 300}, true, true, false);
    SUCCEED();
}

TEST(UICanvas, FocusNextCyclesFocusable) {
    UICanvas c; c.set_screen(800, 600);
    auto a = std::make_shared<Button>();
    auto b = std::make_shared<Button>();
    auto label = std::make_shared<Label>(); // not focusable
    c.add(a); c.add(label); c.add(b);

    c.focus_next(); EXPECT_TRUE(a->focused);
    c.focus_next(); EXPECT_TRUE(b->focused); EXPECT_FALSE(a->focused);
    c.focus_next(); EXPECT_TRUE(a->focused); // wraps, skipping the Label
}

// ── Grid ────────────────────────────────────────────────────────────────────

TEST(Grid, LaysChildrenIntoCells) {
    auto grid = std::make_shared<Grid>(2, Vec2{10, 10});
    grid->size = {0, 0}; // fill
    std::vector<std::shared_ptr<Panel>> kids;
    for (int i = 0; i < 4; ++i) {
        auto p = std::make_shared<Panel>();
        kids.push_back(p);
        grid->add_child(p);
    }
    grid->resolve_layout({0, 0, 210, 210}); // cells 100x100 with 10px gaps

    EXPECT_FLOAT_EQ(kids[0]->rect().x, 0.f);
    EXPECT_FLOAT_EQ(kids[0]->rect().w, 100.f);
    EXPECT_FLOAT_EQ(kids[1]->rect().x, 110.f);
    EXPECT_FLOAT_EQ(kids[2]->rect().y, 110.f);
    EXPECT_FLOAT_EQ(kids[3]->rect().x, 110.f);
    EXPECT_FLOAT_EQ(kids[3]->rect().y, 110.f);
}

TEST(Grid, ChildWithExplicitSizeAnchorsWithinCell) {
    auto grid = std::make_shared<Grid>(1, Vec2{0, 0});
    grid->size = {0, 0};
    auto p = std::make_shared<Panel>();
    p->anchor = {0.5f, 0.5f}; p->pivot = {0.5f, 0.5f}; p->size = {20, 20};
    grid->add_child(p);
    grid->resolve_layout({0, 0, 100, 100}); // one cell = whole rect

    EXPECT_FLOAT_EQ(p->rect().x, 40.f); // centred 20 in 100
    EXPECT_FLOAT_EQ(p->rect().w, 20.f);
}

// ── Button state colours ────────────────────────────────────────────────────

TEST(Button, BackgroundReflectsState) {
    Button b;
    b.bg          = Color(0.1f, 0, 0, 1);
    b.bg_hover    = Color(0.2f, 0, 0, 1);
    b.bg_pressed  = Color(0.3f, 0, 0, 1);
    b.bg_disabled = Color(0.4f, 0, 0, 1);

    EXPECT_FLOAT_EQ(b.current_background().r, 0.1f);
    b.hovered = true;
    EXPECT_FLOAT_EQ(b.current_background().r, 0.2f);
    b.pressed = true;
    EXPECT_FLOAT_EQ(b.current_background().r, 0.3f);
    b.enabled = false;
    EXPECT_FLOAT_EQ(b.current_background().r, 0.4f); // disabled overrides
}
