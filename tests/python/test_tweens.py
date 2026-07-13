"""Tweens: move a value from A to B over time, along an easing curve."""
import pytest

import loom2d as loom

ALL_EASINGS = [e for e in loom.Ease.__members__.values()]


def test_every_easing_starts_at_zero_and_ends_at_one():
    for easing in ALL_EASINGS:
        assert loom.ease(easing, 0.0) == pytest.approx(0.0, abs=1e-5)
        assert loom.ease(easing, 1.0) == pytest.approx(1.0, abs=1e-5)


def test_ease_shapes_the_curve():
    assert loom.ease(loom.Ease.Linear, 0.5) == pytest.approx(0.5)
    assert loom.ease(loom.Ease.InQuad, 0.5) < 0.5      # starts slow
    assert loom.ease(loom.Ease.OutQuad, 0.5) > 0.5     # ends slow


def test_tween_moves_a_node_attribute():
    node = loom.Node()
    node.x = 0.0
    tweens = loom.TweenManager()
    tweens.to(node, "x", 100.0, 1.0)

    tweens.update(0.5)
    assert node.x == pytest.approx(50.0, abs=0.01)
    tweens.update(0.5)
    assert node.x == pytest.approx(100.0, abs=0.01)
    assert tweens.count == 0                            # finished, and dropped


def test_tween_starts_from_wherever_the_attribute_is_now():
    node = loom.Node()
    node.x = 20.0
    tweens = loom.TweenManager()
    tween = tweens.to(node, "x", 30.0, 1.0)

    tweens.update(0.5)
    assert node.x == pytest.approx(25.0, abs=0.01)
    assert tween.progress == pytest.approx(0.5, abs=0.01)


def test_tween_walks_a_dotted_path():
    # Reaching into a nested attribute — the shape of `sprite.tint.a` fades.
    class Target:
        class Inner:
            alpha = 1.0

        def __init__(self):
            self.inner = Target.Inner()

    target = Target()
    tweens = loom.TweenManager()
    tweens.to(target, "inner.alpha", 0.0, 1.0)

    tweens.update(0.5)
    assert target.inner.alpha == pytest.approx(0.5, abs=0.01)


def test_on_complete_fires_once_at_the_end():
    node = loom.Node()
    tweens = loom.TweenManager()
    done = []
    tweens.to(node, "x", 10.0, 1.0, on_complete=lambda: done.append(1))

    tweens.update(0.5)
    assert done == []
    tweens.update(0.5)
    assert done == [1]
    tweens.update(1.0)
    assert done == [1]


def test_delay_holds_the_tween_before_it_starts():
    node = loom.Node()
    node.x = 5.0
    tweens = loom.TweenManager()
    tweens.to(node, "x", 15.0, 1.0, delay=1.0)

    tweens.update(0.5)
    assert node.x == pytest.approx(5.0)      # still waiting
    tweens.update(1.0)                       # 0.5s into the tween proper
    assert node.x == pytest.approx(10.0, abs=0.01)


def test_cancel_freezes_a_tween_without_completing_it():
    node = loom.Node()
    tweens = loom.TweenManager()
    done = []
    tween = tweens.to(node, "x", 100.0, 1.0, on_complete=lambda: done.append(1))

    tweens.update(0.5)
    assert tweens.cancel(tween)
    tweens.update(0.5)

    assert node.x == pytest.approx(50.0, abs=0.01)   # never reached 100
    assert done == []                                # it never arrived
    assert tween.cancelled
    assert tweens.count == 0


def test_the_tween_keeps_its_target_alive():
    # `tweens.to(Node(), ...)` — Python holds no reference to the node. The same
    # trap as scene.add(Enemy()): if the tween did not hold the target, the node
    # would be collected mid-flight and the per-frame setattr would touch freed
    # memory. Run one to completion with nothing but the engine holding it.
    tweens = loom.TweenManager()
    done = []
    tweens.to(loom.Node(), "x", 100.0, 0.5, on_complete=lambda: done.append(1))

    for _ in range(10):
        tweens.update(0.1)
    assert done == [1]


def test_a_tween_can_be_built_by_hand_and_handed_over():
    tweens = loom.TweenManager()
    seen = []
    tween = loom.Tween(0.0, 10.0, 1.0, loom.Ease.OutQuad)
    tween.on_update = seen.append
    tweens.add(tween)

    tweens.update(1.0)
    assert seen[-1] == pytest.approx(10.0, abs=0.01)
    assert tween.done


def test_on_complete_can_chain_the_next_tween():
    node = loom.Node()
    node.x = 0.0
    tweens = loom.TweenManager()

    def back_again():
        tweens.to(node, "x", 0.0, 1.0)

    tweens.to(node, "x", 10.0, 1.0, on_complete=back_again)

    tweens.update(1.0)
    assert node.x == pytest.approx(10.0, abs=0.01)
    assert tweens.count == 1              # the chained tween is queued
    tweens.update(1.0)
    assert node.x == pytest.approx(0.0, abs=0.01)


def test_clear_cancels_everything_in_flight():
    node = loom.Node()
    tweens = loom.TweenManager()
    tweens.to(node, "x", 100.0, 1.0)
    tweens.to(node, "y", 100.0, 1.0)
    assert tweens.count == 2

    tweens.clear()
    assert tweens.count == 0
    tweens.update(1.0)
    assert node.x == pytest.approx(0.0)
    assert node.y == pytest.approx(0.0)


def test_game_drives_tweens():
    game = loom.Game()
    node = loom.Node()
    game.tweens.to(node, "x", 60.0, 1.0)

    game.tweens.update(1.0)
    assert node.x == pytest.approx(60.0, abs=0.01)
