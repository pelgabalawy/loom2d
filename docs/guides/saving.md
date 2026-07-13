# Saving & Loading

`loom.SaveFile` reads and writes a JSON file in the directory the operating
system set aside for your game.

```python
import loom2d as loom

class MyGame(loom.Game):
    def on_start(self):
        self.save = loom.SaveFile("Acme", "Coin Quest")
        self.progress = self.save.load({"level": 1, "coins": 0, "unlocked": []})

    def on_level_complete(self):
        self.progress["level"] += 1
        self.save.save(self.progress)
```

That is the whole API. `load()` returns whatever you saved — a plain `dict`, so
anything JSON can hold: numbers, strings, booleans, lists, nested dicts.

## Where the file goes

Your game does not choose. Next to the executable is wrong (the install directory
is usually read-only, and on macOS it is inside a signed bundle), the working
directory is wrong (it is wherever the player happened to launch from), and the
home directory is wrong everywhere but Linux.

`loom.save_dir(org, app)` asks the OS, which means the same line of code lands in
the right place on every platform — and creates it if it isn't there yet:

| Platform | Where saves go |
|----------|----------------|
| Windows | `%APPDATA%\Acme\Coin Quest\` |
| macOS | `~/Library/Application Support/Acme/Coin Quest/` |
| Linux | `$XDG_DATA_HOME/Acme/Coin Quest/` (usually `~/.local/share/…`) |
| Android / iOS | inside the app's own sandbox |

`save.path` is the full path to the file, which is worth printing while you are
developing.

## A missing save and a broken one are the same thing

`load(default)` returns `default` when there is no save file — **and** when there
is one it cannot parse. A truncated or hand-edited file is not something to crash
the player's game over: there is nothing to resume from, which is precisely the
situation a new game is in. So there is no "is this the first run" branch to
write:

```python
self.progress = self.save.load({"level": 1, "coins": 0})
```

## Writes are atomic

`save()` writes to a temporary file in the same directory and then moves it into
place. A crash — or a pulled power cable — in the middle of a save leaves the
**previous** save intact, rather than a half-written file that reads as corrupt.
That matters more than it sounds: autosaving on a level boundary is exactly when
a game is most likely to be doing something slow enough to be killed.

## Several files, one game

Pass a `name` to keep saves apart. They share the game's directory:

```python
settings = loom.SaveFile("Acme", "Coin Quest", "settings.json")
slot1    = loom.SaveFile("Acme", "Coin Quest", "slot1.json")
slot2    = loom.SaveFile("Acme", "Coin Quest", "slot2.json")
```

| Member | Does |
|--------|------|
| `save.load(default=None)` | parse the file, or return `default` if it is missing or unreadable |
| `save.save(data)` | write `data` as JSON, atomically |
| `save.exists` | is there a save file |
| `save.delete()` | remove it; `False` if there wasn't one |
| `save.path` / `save.directory` | where it lives |

`SaveFile(..., directory=...)` overrides the location outright — useful for tests
and for portable installs that keep everything in one folder.

## Saving things that aren't JSON

Turn them into something that is. A `Vec2` is two floats, a `Color` is four —
save them as lists and rebuild them on load. Do not pickle: a pickle is tied to
your class layout, so a save from an older version of the game will refuse to
load into a newer one, which is the one thing a save file must never do.

```python
data = {"pos": [hero.x, hero.y]}
...
hero.x, hero.y = data["pos"]
```

## See it running

`examples/timers_tweens/` keeps a running total across separate launches — run it
twice and the "all runs" counter carries over.
