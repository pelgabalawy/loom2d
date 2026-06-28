"""
Test the Game base class Python API (no window opened).
"""
import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Game, Color, ScaleMode


class TestGameSubclassing:
    def test_game_is_instantiable(self):
        g = Game()
        assert g is not None

    def test_subclass_default_methods(self):
        class MyGame(Game):
            pass

        g = MyGame()
        g.on_start()
        g.on_update(0.016)
        g.on_draw()
        g.on_stop()

    def test_on_start_called(self):
        log = []

        class MyGame(Game):
            def on_start(self):
                log.append("start")

        g = MyGame()
        g.on_start()
        assert log == ["start"]

    def test_on_update_receives_dt(self):
        received = []

        class MyGame(Game):
            def on_update(self, dt):
                received.append(dt)

        g = MyGame()
        g.on_update(0.016)
        assert received == [pytest.approx(0.016)]

    def test_on_draw_called(self):
        log = []

        class MyGame(Game):
            def on_draw(self):
                log.append("draw")

        g = MyGame()
        g.on_draw()
        assert log == ["draw"]

    def test_on_stop_called(self):
        log = []

        class MyGame(Game):
            def on_stop(self):
                log.append("stop")

        g = MyGame()
        g.on_stop()
        assert log == ["stop"]

    def test_clear_color_default(self):
        g = Game()
        c = g.clear_color
        # Default is cornflower blue — roughly (0.39, 0.58, 0.93)
        assert c.r == pytest.approx(0.39, abs=0.01)

    def test_clear_color_settable(self):
        g = Game()
        g.clear_color = Color.red()
        assert g.clear_color.r == pytest.approx(1.0)
        assert g.clear_color.g == pytest.approx(0.0)

    def test_auto_flags(self):
        g = Game()
        assert g.auto_physics is True
        assert g.auto_scene is True

    def test_game_has_scene(self):
        g = Game()
        assert g.scene is not None

    def test_game_has_physics(self):
        g = Game()
        assert g.physics is not None

    def test_game_has_audio(self):
        g = Game()
        assert g.audio is not None

    def test_game_has_assets(self):
        g = Game()
        assert g.assets is not None

    def test_state_persists_between_calls(self):
        class Counter(Game):
            def __init__(self):
                super().__init__()
                self.count = 0

            def on_update(self, dt):
                self.count += 1

        g = Counter()
        g.on_update(0.016)
        g.on_update(0.016)
        assert g.count == 2

    def test_multiple_subclasses_independent(self):
        class A(Game):
            val = 1
        class B(Game):
            val = 2

        assert A().val == 1
        assert B().val == 2


class TestResponsiveScaling:
    def test_logical_size_defaults_to_zero(self):
        # 0 means "use the window size" — run() resolves it on startup.
        g = Game()
        assert g.logical_width == 0
        assert g.logical_height == 0

    def test_logical_size_settable(self):
        g = Game()
        g.logical_width = 1280
        g.logical_height = 720
        assert g.logical_width == 1280
        assert g.logical_height == 720

    def test_scale_mode_default_is_fit(self):
        g = Game()
        assert g.scale_mode == ScaleMode.Fit

    def test_scale_mode_settable(self):
        g = Game()
        g.scale_mode = ScaleMode.PixelPerfect
        assert g.scale_mode == ScaleMode.PixelPerfect

    def test_scale_mode_enum_values(self):
        names = {ScaleMode.Fit, ScaleMode.Stretch,
                 ScaleMode.Expand, ScaleMode.PixelPerfect}
        assert len(names) == 4

    def test_screen_size_readonly_default_zero(self):
        # Updated by run() each frame; zero until a window exists.
        g = Game()
        assert g.screen_width == 0
        assert g.screen_height == 0

    def test_on_resize_overridable(self):
        events = []

        class MyGame(Game):
            def on_resize(self, w, h):
                events.append((w, h))

        g = MyGame()
        g.on_resize(1024, 768)
        assert events == [(1024, 768)]
