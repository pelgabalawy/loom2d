"""
Test the UI toolkit (Phase 2.9) headlessly.

Layout, hit-testing, pointer dispatch and focus are all GPU-free, so the whole
widget behaviour is testable without a window. Actual widget *rendering* needs a
real Renderer (atlas/white-texture upload) and is exercised by examples/ui_demo.
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import (
    Vec2, Rect, Color,
    Widget, Panel, Label, Button, Image, Grid, UICanvas,
)


def _canvas(w=800, h=600):
    c = UICanvas()
    c.set_screen(w, h)
    return c


class TestAnchoredLayout:
    def test_top_left_default(self):
        w = Widget()
        w.size = Vec2(100, 40)
        w.resolve_layout(Rect(0, 0, 800, 600))
        r = w.rect()
        assert (r.x, r.y, r.w, r.h) == (0, 0, 100, 40)

    def test_centered(self):
        w = Widget()
        w.size = Vec2(100, 40)
        w.anchor = Vec2(0.5, 0.5)
        w.pivot = Vec2(0.5, 0.5)
        w.resolve_layout(Rect(0, 0, 800, 600))
        r = w.rect()
        assert (r.x, r.y) == (350, 280)

    def test_bottom_right_inset(self):
        w = Widget()
        w.size = Vec2(100, 40)
        w.anchor = Vec2(1, 1)
        w.pivot = Vec2(1, 1)
        w.offset = Vec2(-10, -10)
        w.resolve_layout(Rect(0, 0, 800, 600))
        r = w.rect()
        assert (r.right(), r.bottom()) == (790, 590)

    def test_zero_size_fills_parent(self):
        w = Widget()  # size defaults to (0,0)
        w.resolve_layout(Rect(5, 7, 800, 600))
        r = w.rect()
        assert (r.x, r.y, r.w, r.h) == (5, 7, 800, 600)

    def test_negative_size_insets_parent(self):
        w = Widget()
        w.size = Vec2(-40, -20)
        w.resolve_layout(Rect(0, 0, 800, 600))
        r = w.rect()
        assert (r.w, r.h) == (760, 580)


class TestTree:
    def test_add_child_recurses_layout(self):
        parent = Panel()
        child = Panel()
        child.anchor = Vec2(0.5, 0.5)
        child.pivot = Vec2(0.5, 0.5)
        child.size = Vec2(50, 50)
        parent.add_child(child)
        parent.resolve_layout(Rect(0, 0, 200, 200))
        assert child.rect().x == 75 and child.rect().y == 75

    def test_children_listing(self):
        parent = Widget()
        a, b = Panel(), Panel()
        parent.add_child(a)
        parent.add_child(b)
        assert len(parent.children()) == 2


class TestPointerDispatch:
    def _centred_button(self, canvas):
        b = Button()
        b.anchor = Vec2(0.5, 0.5)
        b.pivot = Vec2(0.5, 0.5)
        b.size = Vec2(100, 40)
        self.clicks = 0

        def _click():
            self.clicks += 1

        b.on_clicked = _click
        canvas.add(b)
        canvas.layout()
        return b

    def test_click_on_press_release(self):
        c = _canvas()
        b = self._centred_button(c)
        c.update_input(Vec2(400, 300), True, True, False)
        assert b.hovered and b.pressed and b.focused
        c.update_input(Vec2(400, 300), False, False, True)
        assert self.clicks == 1
        assert not b.pressed

    def test_no_click_when_released_outside(self):
        c = _canvas()
        self._centred_button(c)
        c.update_input(Vec2(400, 300), True, True, False)
        c.update_input(Vec2(10, 10), False, False, True)
        assert self.clicks == 0

    def test_pressed_only_while_over_target(self):
        c = _canvas()
        b = self._centred_button(c)
        c.update_input(Vec2(400, 300), True, True, False)
        assert b.pressed
        c.update_input(Vec2(10, 10), False, True, False)  # dragged off, held
        assert not b.pressed
        c.update_input(Vec2(400, 300), False, True, False)  # back on
        assert b.pressed

    def test_disabled_widget_ignores_input(self):
        c = _canvas()
        b = self._centred_button(c)
        b.enabled = False
        c.layout()
        c.update_input(Vec2(400, 300), True, True, False)
        c.update_input(Vec2(400, 300), False, False, True)
        assert self.clicks == 0
        assert not b.focused


class TestFocus:
    def test_click_focuses_focusable_and_clears_on_empty(self):
        c = _canvas()
        b = Button()
        b.anchor = Vec2(0.5, 0.5)
        b.pivot = Vec2(0.5, 0.5)
        b.size = Vec2(100, 40)
        c.add(b)
        c.layout()
        c.update_input(Vec2(400, 300), True, True, False)
        assert b.focused
        c.update_input(Vec2(5, 5), True, True, False)  # empty corner
        assert not b.focused

    def test_focus_next_cycles_skipping_non_focusable(self):
        c = _canvas()
        a, b, label = Button(), Button(), Label()
        c.add(a)
        c.add(label)
        c.add(b)
        c.focus_next()
        assert a.focused
        c.focus_next()
        assert b.focused and not a.focused
        c.focus_next()
        assert a.focused  # wraps, skipping the Label

    def test_remove_clears_focus(self):
        c = _canvas()
        b = Button()
        b.anchor = Vec2(0.5, 0.5)
        b.pivot = Vec2(0.5, 0.5)
        b.size = Vec2(100, 40)
        c.add(b)
        c.layout()
        c.update_input(Vec2(400, 300), True, True, False)
        assert b.focused
        c.remove(b)
        c.layout()
        c.update_input(Vec2(400, 300), True, True, False)  # must not crash
        assert b.focusable  # widget object still alive in Python


class TestGrid:
    def test_cells(self):
        grid = Grid(2, Vec2(10, 10))
        kids = [Panel() for _ in range(4)]
        for k in kids:
            grid.add_child(k)
        grid.resolve_layout(Rect(0, 0, 210, 210))
        assert kids[0].rect().w == 100
        assert kids[1].rect().x == 110
        assert kids[2].rect().y == 110
        assert (kids[3].rect().x, kids[3].rect().y) == (110, 110)

    def test_child_with_size_anchored_in_cell(self):
        grid = Grid(1)
        p = Panel()
        p.anchor = Vec2(0.5, 0.5)
        p.pivot = Vec2(0.5, 0.5)
        p.size = Vec2(20, 20)
        grid.add_child(p)
        grid.resolve_layout(Rect(0, 0, 100, 100))
        assert p.rect().x == 40 and p.rect().w == 20


class TestButtonState:
    def test_current_background(self):
        b = Button()
        b.bg = Color(0.1, 0, 0, 1)
        b.bg_hover = Color(0.2, 0, 0, 1)
        b.bg_pressed = Color(0.3, 0, 0, 1)
        b.bg_disabled = Color(0.4, 0, 0, 1)
        assert b.current_background().r == pytest.approx(0.1)
        b.enabled = False
        assert b.current_background().r == pytest.approx(0.4)

    def test_button_is_focusable_by_default(self):
        assert Button().focusable
        assert not Label().focusable


class TestBindingsSurface:
    def test_image_holds_optional_source(self):
        img = Image()
        img.tint = Color.red()
        img.source = Rect(0, 0, 16, 16)
        assert img.source.w == 16

    def test_canvas_screen_dims(self):
        c = _canvas(640, 360)
        assert (c.screen_width, c.screen_height) == (640, 360)
