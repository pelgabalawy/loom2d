# Audio

loom2d plays sound effects and streaming music through
[miniaudio](https://github.com/mackron/miniaudio). Your `Game` owns a ready-to-use
`AudioEngine` as `self.audio`.

## Sound effects

Fire-and-forget one-shot sounds — good for jumps, hits, pickups:

```python
class Game(loom.Game):
    def on_update(self, dt):
        if loom.Input.key_pressed(loom.Key.Space):
            self.audio.play_sound("sfx/jump.wav", volume=0.8)
```

`play_sound(path, volume=1.0)` loads and plays a clip. Call it as often as you like;
overlapping sounds mix together.

## Music

Streaming background music with looping:

```python
def on_start(self):
    self.audio.play_music("music/theme.ogg", volume=0.5, loop=True)
```

| Method | Description |
|--------|-------------|
| `play_music(path, volume=1.0, loop=True)` | start (or replace) the streaming track |
| `stop_music()` | stop the current track |
| `set_music_volume(v)` | change music volume `[0,1]` on the fly |
| `music_playing()` | `bool` — is a track currently playing? |

```python
self.audio.set_music_volume(0.2)   # duck the music
if not self.audio.music_playing():
    self.audio.play_music("music/next.ogg")
```

## Master volume

Scale everything at once — handy for a global mute or a settings slider:

```python
self.audio.set_master_volume(0.0)   # mute all audio
self.audio.set_master_volume(1.0)   # full volume
```

## Checking availability

If no audio device is available (some CI machines, headless boxes), the engine
degrades gracefully. You can check:

```python
if self.audio.initialized:
    self.audio.play_music("music/theme.ogg")
```

!!! note "Supported formats"
    miniaudio decodes common formats including **WAV**, **MP3**, **FLAC**, and **OGG**
    Vorbis. Use short uncompressed clips (WAV) for SFX and a compressed format (OGG) for
    long music tracks.

## See also

- [The Game Loop](../getting-started/game-loop.md) — `self.audio` is created for you
- [Assets](assets.md) — for textures (audio is loaded by path on play)
