# Scenes & Transitions

A **scene** is both a container and a game state: it owns a node tree, a camera
and its own UI layer, and it has lifecycle hooks you override. A menu, a level
and a game-over screen are three scenes, and `game.scenes` moves between them.

```python
import loom2d as loom

class MenuScene(loom.Scene):
    def on_enter(self):
        play = loom.Button(self.game.font, "Play")
        play.anchor = loom.Vec2(0.5, 0.5)
        play.pivot  = loom.Vec2(0.5, 0.5)
        play.size   = loom.Vec2(160, 48)
        play.on_clicked = self.play
        self.ui.add(play)

    def play(self):
        self.game.scenes.switch_to(LevelScene(), loom.Fade(0.4))


class LevelScene(loom.Scene):
    def on_enter(self):
        self.hero = loom.SpriteNode(self.game.assets.texture("hero.png"))
        self.add(self.hero)

    def on_update(self, dt):
        self.hero.x += 60 * dt


class MyGame(loom.Game):
    def on_start(self):
        self.font = loom.Font.load("C:/Windows/Fonts/arial.ttf", 18)
        self.scenes.switch_to(MenuScene())
```

## Lifecycle

| Hook | When |
|------|------|
| `on_enter()` | the scene becomes active |
| `on_update(dt)` | once a frame, while it is on **top** of the stack |
| `on_draw()` | once a frame, after its nodes are drawn |
| `on_exit()` | it is replaced or popped, before it is released |

Build your scene in `on_enter`, not `__init__` — `self.game` is wired up just
before `on_enter` runs, so that is the first moment you can reach the game's
assets, audio and physics.

## Each scene owns its own UI

`scene.ui` is a `UICanvas` exactly like `game.ui`, but it belongs to the scene:
the menu's buttons and the level's HUD never share one canvas, and a scene's
widgets disappear with it. `game.ui` still exists for genuinely global chrome
(an FPS counter, a debug overlay) and is drawn **above** every scene.

When a pointer lands on a `game.ui` widget it does **not** also fall through to
the scene's UI underneath.

## Switching, pushing, popping

```python
game.scenes.switch_to(LevelScene())   # replace the active scene
game.scenes.push(PauseScene())        # lay a scene OVER the active one
game.scenes.pop()                     # drop the top scene
```

`switch_to` is the usual move — menu to level, level to game-over.

`push` is what makes a **pause menu** or a dialogue box trivial. The scene below
stays alive and keeps **drawing**, but stops **updating** — it freezes exactly
where it was. `pop` reveals it again, untouched, with no second `on_enter`:

```python
class LevelScene(loom.Scene):
    def on_update(self, dt):
        if loom.Input.key_pressed(loom.Key.P):
            self.game.scenes.push(PauseScene())   # the level freezes here


class PauseScene(loom.Scene):
    def on_enter(self):
        dim = loom.Panel(loom.Color(0, 0, 0, 0.6))   # the level shows through
        self.ui.add(dim)

    def resume(self):
        self.game.scenes.pop()                       # ...and carries on here
```

The stack is never empty: `pop()` on the last scene is a no-op, so `game.scene`
is always valid.

## Transitions

Pass a transition and the swap is deferred to the animation's **midpoint**, so
the cut happens while the screen is covered:

```python
game.scenes.switch_to(LevelScene(), loom.Fade(0.4))          # fade through black
game.scenes.switch_to(LevelScene(), loom.Fade(0.4, loom.Color.white()))
```

`Fade(duration, color)` fades out to a colour over the first half, swaps, then
fades back in. Until the midpoint the **outgoing** scene is still the live one —
it keeps updating and animating as it fades out, and `game.scene` still returns
it. `game.scenes.transitioning` tells you whether one is in flight; input is
withheld from a scene's UI while it is, so a click cannot fire the wrong menu
mid-fade.

A fade is drawn over **everything**, including `game.ui`.

Write your own by subclassing `loom.Transition` and implementing `update(dt)`,
`swap_ready()`, `done()` and `draw(renderer, w, h)`.

!!! note "Cross-fades"
    A true cross-fade — both scenes on screen at once — needs render-to-texture,
    and arrives with `Canvas` support in the rendering phase.

## `game.scene`

`game.scene` is the **active** scene (the top of the stack), so the simple case
still needs no scene management at all:

```python
class MyGame(loom.Game):
    def on_start(self):
        self.scene.add(loom.SpriteNode(tex))   # the default scene
```

To change scenes, always go through `game.scenes` — that is what runs the
lifecycle hooks and the transitions.

## The stack in one table

| Call | Active scene | Scene below |
|------|--------------|-------------|
| `switch_to(s)` | replaced by `s` | — (the old one exits) |
| `push(s)` | `s` | alive, drawn, **frozen** |
| `pop()` | the one below | resumes updating |
