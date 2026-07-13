# Timers & Tweens

Two ways to make something happen over time without writing a state machine in
`on_update`.

* **`game.timers`** runs a callback **later**, once or on a repeat.
* **`game.tweens`** moves a **value** from where it is to where you want it,
  along an easing curve.

Both are driven by the run loop, so there is nothing to step yourself.

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        self.hero = loom.SpriteNode(self.assets.texture("hero.png"))
        self.scene.add(self.hero)

        # Slide the hero in, with a bit of overshoot at the end.
        self.tweens.to(self.hero, "x", 400, 0.6, loom.Ease.OutBack)

        # Then, half a second later, start a wave every three seconds.
        self.timers.after(0.5, self.spawn_wave)
        self.timers.every(3.0, self.spawn_wave)
```

## Timers

| Call | Does |
|------|------|
| `timers.after(delay, fn)` | call `fn` once, `delay` seconds from now |
| `timers.every(interval, fn, times=0)` | call `fn` every `interval` seconds; `times=0` repeats for ever |
| `timers.cancel(handle)` | stop a timer; `False` if it was unknown or already done |
| `timers.clear()` | stop every timer |
| `timers.active(handle)` / `timers.count` | is it still scheduled / how many are |

`after` and `every` return a **handle** — keep it if you might want to cancel:

```python
class Boss(loom.Node):
    def enrage(self):
        self.shots = self.game.timers.every(0.4, self.fire)

    def die(self):
        self.game.timers.cancel(self.shots)
```

A callback can schedule and cancel timers freely, including its own handle — a
repeat that stops itself once a condition is met is the normal way to write a
countdown:

```python
def tick(self):
    self.seconds -= 1
    self.label.text = str(self.seconds)
    if self.seconds == 0:
        self.game.timers.cancel(self.countdown)
        self.game_over()

self.countdown = self.timers.every(1.0, self.tick)
```

Timers are driven with the frame's `dt`, and a repeat **carries its overshoot**:
a `every(1.0)` timer on a machine dropping frames still fires 60 times a minute
rather than drifting behind. A frame long enough to have missed several fires
catches up on all of them in that frame.

## Tweens

`tweens.to()` names an attribute and where it should end up:

```python
game.tweens.to(sprite, "x", 400, 0.6, loom.Ease.OutBack)
game.tweens.to(sprite, "rotation", 6.28, 1.0)
game.tweens.to(panel, "background.a", 0.0, 0.5)   # dotted paths reach inside
```

| Argument | Means |
|----------|-------|
| `target`, `prop` | the object and the **float** attribute to animate; `prop` may be dotted (`"tint.a"`, `"offset.y"`) |
| `to` | the value to arrive at — it starts from whatever the attribute reads **now** |
| `duration` | seconds |
| `easing` | an `Ease` curve; `Ease.Linear` by default |
| `delay` | wait this long before moving |
| `on_complete` | called the frame it arrives |

The tween holds a reference to its target, so `tweens.to(Enemy(), ...)` is safe
even if your game keeps none. It is dropped the moment it finishes — finished
tweens do not pile up.

Chain from `on_complete` to build a sequence. Nothing here touches `on_update`:

```python
def pulse(self, to_alpha):
    self.tweens.to(self.hint, "color.a", to_alpha, 0.9, loom.Ease.InOutSine,
                   on_complete=lambda: self.pulse(1.0 if to_alpha < 0.5 else 0.25))
```

`tweens.to()` returns the `Tween`, which you can `cancel()` — it freezes where it
is, and `on_complete` does **not** fire, because it never arrived. `tweens.clear()`
cancels everything in flight.

!!! note "Only floats"
    A tween animates one float. `node.x`, `node.y`, `rotation`, an alpha — yes.
    `position` (a `Vec2`) — no: tween `"x"` and `"y"` as two tweens, or reach in
    with a dotted path. This is also the faster way round; see the note on `Vec2`
    allocation in [Sprites](sprites.md).

## Easing curves

`Ease.Linear` plus `In`/`Out`/`InOut` of **Quad, Cubic, Quart, Sine, Expo, Circ,
Back, Elastic** and **Bounce** — the standard curves, so they behave exactly as
they do in every other engine.

| Curve | Feels like |
|-------|-----------|
| `Linear` | mechanical — a lift, a conveyor |
| `OutQuad` / `OutCubic` | the everyday choice: quick, then settles. UI, camera moves |
| `InQuad` / `InCubic` | winding up — something starting to fall |
| `InOutSine` / `InOutQuad` | smooth both ends; breathing, drifting |
| `OutBack` | overshoots and settles back — pop-in menus, button presses |
| `OutElastic` | wobbles into place — rubbery, cartoonish |
| `OutBounce` | lands and bounces — anything dropping onto the ground |
| `InExpo` / `OutExpo` | extreme acceleration; a dash, a slam |

`loom.ease(curve, t)` gives you the raw curve for a normalised time `0..1` if you
want to shape something by hand.

`Back` and `Elastic` deliberately **overshoot** — they leave the `from`→`to` range
in the middle and come back. That is what makes them feel alive, but it does mean
a `tweens.to(hp_bar, "width", ...)` on `OutBack` will briefly draw past full.

## Running them yourself

Both managers are stepped by the run loop before physics and scenes, so a timer
callback that moves something has it simulated and drawn the same frame. Turn
either off with `game.auto_timers = False` / `game.auto_tweens = False` and call
`timers.update(dt)` / `tweens.update(dt)` when it suits you — a game that pauses
by freezing its own clock does exactly this.

## See it running

`examples/timers_tweens/` is an easing gallery: eight squares racing on eight
curves, relaunched by a repeating timer, with a pulsing label chained out of
`on_complete` — and a save file counting the sweeps across runs.

```bash
python examples/timers_tweens/main.py
```
