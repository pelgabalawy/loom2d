"""Audio bindings: AudioEngine + SoundHandle.

These cover the one-shot SFX path — `audio.play_sound(path)` — which is the most
common audio call in a game and which used to be completely broken from Python:

  * `SoundHandle` was never registered with pybind11, so play_sound() raised on
    return-value conversion, and
  * the ma_sound was held by a shared_ptr with the DEFAULT deleter, so releasing
    it freed the sound without ma_sound_uninit() while miniaudio's audio thread
    was still mixing it — a hard crash.

The tests skip when there's no audio device (headless CI).
"""
import struct
import wave

import pytest

import loom2d as loom


@pytest.fixture
def tone(tmp_path):
    """A short mono 16-bit WAV that actually decodes.

    Playing a *nonexistent* file bails out early and never exercises the
    interesting path, which is exactly how the crash above went unnoticed.
    """
    path = tmp_path / "tone.wav"
    with wave.open(str(path), "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(8000)
        frames = b"".join(
            struct.pack("<h", 3000 if (i // 10) % 2 else -3000) for i in range(2000)
        )
        w.writeframes(frames)
    return str(path)


@pytest.fixture
def engine():
    e = loom.AudioEngine()
    if not e.initialized:
        pytest.skip("no audio device available in this environment")
    return e


class TestAudioEngine:
    def test_constructs(self):
        loom.AudioEngine()  # never throws, even with no device

    def test_music_not_playing_by_default(self):
        assert loom.AudioEngine().music_playing() is False

    def test_master_volume(self):
        e = loom.AudioEngine()
        e.set_master_volume(0.5)
        e.set_master_volume(0.0)
        e.set_master_volume(1.0)


class TestPlaySound:
    def test_returns_a_sound_handle(self, engine, tone):
        handle = engine.play_sound(tone, 0.05)
        assert isinstance(handle, loom.SoundHandle)

    def test_handle_exposes_controls(self, engine, tone):
        handle = engine.play_sound(tone, 0.05)
        assert isinstance(handle.playing, bool)
        handle.set_volume(0.5)
        handle.stop()

    def test_fire_and_forget_is_safe(self, engine, tone):
        # The handle is optional: the engine keeps the sound alive until it
        # finishes. Dropping it must not free a sound that is still playing.
        for _ in range(32):
            engine.play_sound(tone, 0.05)

    def test_missing_file_returns_a_silent_handle(self, engine):
        handle = engine.play_sound("no_such_file.wav")
        assert handle.playing is False

    def test_handle_may_outlive_the_engine(self, tone):
        # Regression: the sound's deleter runs ma_sound_uninit(), which must not
        # execute against a destroyed ma_engine. Dropping the engine while a
        # handle is still held used to crash, so every sound keeps its engine
        # alive until the sound itself is released.
        engine = loom.AudioEngine()
        if not engine.initialized:
            pytest.skip("no audio device available in this environment")
        handle = engine.play_sound(tone, 0.05)
        del engine          # engine goes first...
        del handle          # ...sound released afterwards: must not crash

    def test_stopped_sounds_do_not_accumulate(self, engine, tone):
        # Sounds stopped early never reach at_end(); reaping on that alone would
        # hold them for the engine's whole lifetime.
        for _ in range(16):
            engine.play_sound(tone, 0.05).stop()


class TestMusic:
    def test_play_and_stop(self, engine, tone):
        engine.play_music(tone, 0.5, False)
        engine.set_music_volume(0.2)
        engine.stop_music()
        assert engine.music_playing() is False

    def test_missing_file_does_not_play(self, engine):
        engine.play_music("no_such_file.wav")
        assert engine.music_playing() is False

    def test_stop_when_not_playing(self, engine):
        engine.stop_music()
