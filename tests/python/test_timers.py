"""Timers: run a callback later, or on a repeat."""
import loom2d as loom


def test_after_fires_once_when_the_delay_elapses():
    timers = loom.Timers()
    calls = []
    timers.after(1.0, lambda: calls.append("boom"))

    timers.update(0.5)
    assert calls == []
    timers.update(0.5)
    assert calls == ["boom"]

    timers.update(10.0)
    assert calls == ["boom"]          # and never again
    assert timers.count == 0


def test_every_repeats_until_cancelled():
    timers = loom.Timers()
    ticks = []
    handle = timers.every(0.25, lambda: ticks.append(1))

    for _ in range(4):
        timers.update(0.25)
    assert len(ticks) == 4
    assert timers.active(handle)

    assert timers.cancel(handle)
    timers.update(1.0)
    assert len(ticks) == 4
    assert not timers.active(handle)


def test_every_with_a_count_retires_itself():
    timers = loom.Timers()
    ticks = []
    timers.every(0.1, lambda: ticks.append(1), times=3)

    timers.update(1.0)                # long enough for far more than three
    assert len(ticks) == 3
    assert timers.count == 0


def test_a_timer_lives_without_python_keeping_a_reference():
    # The engine owns the callback: a game writes `timers.after(1, self.spawn)`
    # and keeps nothing. If the C++ side did not hold the callable, this would
    # either not fire or crash.
    timers = loom.Timers()
    calls = []

    def make():
        timers.after(0.5, lambda: calls.append("fired"))

    make()
    timers.update(0.5)
    assert calls == ["fired"]


def test_a_callback_can_schedule_and_cancel_timers():
    timers = loom.Timers()
    order = []
    state = {}

    def second():
        order.append("second")
        timers.cancel(state["repeat"])

    def first():
        order.append("first")
        timers.after(1.0, second)

    state["repeat"] = timers.every(1.0, lambda: order.append("tick"))
    timers.after(1.0, first)

    timers.update(1.0)                # first + the repeat's first tick
    assert order == ["tick", "first"]
    timers.update(1.0)                # the repeat ticks, then second cancels it
    assert order == ["tick", "first", "tick", "second"]
    timers.update(5.0)                # nothing left running
    assert order == ["tick", "first", "tick", "second"]
    assert timers.count == 0


def test_clear_stops_everything():
    timers = loom.Timers()
    calls = []
    timers.after(1.0, lambda: calls.append(1))
    timers.every(1.0, lambda: calls.append(2))
    assert timers.count == 2

    timers.clear()
    assert timers.count == 0
    timers.update(5.0)
    assert calls == []


def test_game_drives_timers():
    game = loom.Game()
    calls = []
    game.timers.after(0.5, lambda: calls.append("boom"))

    # `game.timers` is the engine's own Timers, not a copy: updating it through
    # the property is what the run loop does every frame.
    game.timers.update(0.5)
    assert calls == ["boom"]
    assert game.timers.count == 0
