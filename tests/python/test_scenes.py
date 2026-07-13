"""
Scene management from Python (Phase 2.10): lifecycle hooks, the scene stack and
transitions.

All headless — the manager is GPU-free apart from drawing, so a Game can be
constructed and driven with explicit update() calls without opening a window.
"""
import gc

import pytest
loom2d_native = pytest.importorskip("loom2d_native")
from loom2d_native import Game, Scene, Fade, Node


class Traced(Scene):
    """Records its own lifecycle so ordering can be asserted."""

    def __init__(self, name, log):
        super().__init__()
        self.name = name
        self.log = log
        self.updates = 0

    def on_enter(self):
        self.log.append(f"{self.name}:enter")

    def on_exit(self):
        self.log.append(f"{self.name}:exit")

    def on_update(self, dt):
        self.updates += 1


class TestDefaultScene:
    def test_game_starts_with_a_scene(self):
        g = Game()
        assert g.scene is not None
        assert g.scenes.depth == 1

    def test_game_scene_is_the_active_one(self):
        g = Game()
        log = []
        menu = Traced("menu", log)
        g.scenes.switch_to(menu)
        assert g.scene is menu


class TestLifecycle:
    def test_hooks_fire_in_order(self):
        g = Game()
        log = []

        g.scenes.switch_to(Traced("menu", log))
        g.scenes.switch_to(Traced("level", log))

        assert log == ["menu:enter", "menu:exit", "level:enter"]

    def test_on_update_runs_each_frame(self):
        g = Game()
        log = []
        level = Traced("level", log)
        g.scenes.switch_to(level)

        for _ in range(3):
            g.scenes.update(0.016)

        assert level.updates == 3

    def test_scene_can_reach_its_game(self):
        g = Game()
        seen = {}

        class Probe(Scene):
            def on_enter(self):
                # The game must be wired up *before* on_enter, so a scene can
                # load assets and switch scenes from inside it.
                seen["game"] = self.game

        g.scenes.switch_to(Probe())
        assert seen["game"] is g

    def test_scene_survives_when_python_keeps_no_reference(self):
        """`switch_to(MenuScene())` retains no Python reference.

        The manager owns the scene in C++ alone, so without smart_holder
        lifetime support the Python half would be collected and on_update would
        silently stop firing. This is the scene-level case of the bug fixed in
        the Node/Widget hierarchies.
        """
        g = Game()
        ticks = []

        class Ticker(Scene):
            def on_update(self, dt):
                ticks.append(dt)

        g.scenes.switch_to(Ticker())   # no reference kept
        gc.collect()

        g.scenes.update(0.016)
        g.scenes.update(0.016)

        assert len(ticks) == 2
        assert isinstance(g.scene, Ticker)


class TestStack:
    def test_push_overlays_and_pauses_the_scene_below(self):
        g = Game()
        log = []
        level = Traced("level", log)
        pause = Traced("pause", log)

        g.scenes.switch_to(level)
        g.scenes.push(pause)
        assert g.scenes.depth == 2
        assert g.scene is pause

        g.scenes.update(0.016)
        assert pause.updates == 1
        assert level.updates == 0, "the level under a pause menu must freeze"

    def test_pop_reveals_the_scene_below(self):
        g = Game()
        log = []
        level = Traced("level", log)

        g.scenes.switch_to(level)
        g.scenes.push(Traced("pause", log))
        g.scenes.pop()

        assert g.scenes.depth == 1
        assert g.scene is level
        assert log == ["level:enter", "pause:enter", "pause:exit"]

        g.scenes.update(0.016)
        assert level.updates == 1, "the level resumes where it left off"

    def test_pop_never_empties_the_stack(self):
        g = Game()
        g.scenes.pop()
        assert g.scenes.depth == 1
        assert g.scene is not None

    def test_stack_exposes_both_scenes(self):
        g = Game()
        log = []
        g.scenes.switch_to(Traced("level", log))
        g.scenes.push(Traced("pause", log))
        assert len(g.scenes.stack) == 2


class TestTransitions:
    def test_swap_waits_for_the_fade_midpoint(self):
        g = Game()
        log = []
        level = Traced("level", log)
        g.scenes.switch_to(level)
        log.clear()

        g.scenes.switch_to(Traced("next", log), Fade(1.0))
        assert g.scenes.transitioning
        assert g.scene is level, "the outgoing scene stays live while it fades out"
        assert log == []

        g.scenes.update(0.25)
        assert g.scene is level

        g.scenes.update(0.25)      # midpoint — screen is fully covered
        assert log == ["level:exit", "next:enter"]
        assert g.scenes.transitioning, "still fading back in"

        g.scenes.update(0.5)
        assert not g.scenes.transitioning

    def test_fade_alpha_curve(self):
        f = Fade(1.0)
        assert f.alpha == pytest.approx(0.0)
        f.update(0.5)
        assert f.alpha == pytest.approx(1.0)     # fully opaque at the midpoint
        f.update(0.5)
        assert f.alpha == pytest.approx(0.0)

    def test_no_transition_swaps_immediately(self):
        g = Game()
        log = []
        g.scenes.switch_to(Traced("a", log))
        assert not g.scenes.transitioning
        assert log == ["a:enter"]


class TestSceneContents:
    def test_scene_owns_its_nodes_and_ui(self):
        g = Game()
        s = Scene()
        g.scenes.switch_to(s)

        s.add(Node("hero"))
        assert len(s.root().children()) == 1
        assert s.ui is not None

    def test_switching_away_does_not_disturb_the_new_scene(self):
        g = Game()
        first, second = Scene(), Scene()
        first.add(Node("a"))
        second.add(Node("b"))

        g.scenes.switch_to(first)
        g.scenes.switch_to(second)

        assert len(g.scene.root().children()) == 1
        assert g.scene.root().children()[0].name == "b"
