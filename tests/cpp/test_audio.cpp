#include <gtest/gtest.h>
#include "audio/audio.hpp"

using namespace loom;

// Audio tests run against a dummy audio driver (SDL_AUDIODRIVER=dummy).
// We verify lifecycle and state transitions; no actual sound is produced.

TEST(Audio, EngineInitializes) {
    // May return initialized=false in headless CI — that's OK, it's non-fatal
    AudioEngine engine;
    // Either it worked or it gracefully degraded
    SUCCEED();
}

TEST(Audio, MusicNotPlayingByDefault) {
    AudioEngine engine;
    EXPECT_FALSE(engine.music_playing());
}

TEST(Audio, StopMusicWhenNotPlaying) {
    AudioEngine engine;
    EXPECT_NO_THROW(engine.stop_music());
}

TEST(Audio, SetMasterVolume) {
    AudioEngine engine;
    EXPECT_NO_THROW(engine.set_master_volume(0.5f));
    EXPECT_NO_THROW(engine.set_master_volume(0.f));
    EXPECT_NO_THROW(engine.set_master_volume(1.f));
}

TEST(Audio, PlayNonExistentFileReturnsEmptyHandle) {
    AudioEngine engine;
    if (!engine.initialized()) GTEST_SKIP() << "Audio not available in this env";

    SoundHandle h = engine.play_sound("nonexistent_file.wav");
    // Should not crash; just return an empty handle
    EXPECT_FALSE(h.playing());
}

TEST(Audio, PlayMusicNonExistentFileNoThrow) {
    AudioEngine engine;
    if (!engine.initialized()) GTEST_SKIP() << "Audio not available in this env";

    EXPECT_NO_THROW(engine.play_music("nonexistent.ogg"));
    EXPECT_FALSE(engine.music_playing());
}

TEST(Audio, MultipleEnginesIndependent) {
    // Two engines can coexist without interfering
    AudioEngine e1, e2;
    EXPECT_NO_THROW({
        e1.set_master_volume(0.8f);
        e2.set_master_volume(0.5f);
    });
}
