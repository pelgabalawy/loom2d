#include <gtest/gtest.h>
#include "audio/audio.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace loom;

// Audio tests run against a dummy audio driver (SDL_AUDIODRIVER=dummy).
// We verify lifecycle and state transitions; no actual sound is produced.

namespace {

// Write a tiny mono 16-bit PCM WAV. The tests below need a file that ACTUALLY
// loads: playing a nonexistent file bails out early and never exercises the
// interesting path (a live sound being started, then released).
std::string write_temp_wav(const char* name) {
    const uint32_t sample_rate = 8000;
    const uint16_t channels    = 1;
    const uint16_t bits        = 16;

    std::vector<int16_t> samples(2000);
    for (size_t i = 0; i < samples.size(); ++i)
        samples[i] = static_cast<int16_t>((i % 20 < 10) ? 3000 : -3000);

    const uint32_t data_bytes  = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    const uint32_t byte_rate   = sample_rate * channels * bits / 8;
    const uint16_t block_align = channels * bits / 8;

    std::string path(name);
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };

    f.write("RIFF", 4); u32(36 + data_bytes); f.write("WAVE", 4);
    f.write("fmt ", 4); u32(16); u16(1); u16(channels); u32(sample_rate);
    u32(byte_rate); u16(block_align); u16(bits);
    f.write("data", 4); u32(data_bytes);
    f.write(reinterpret_cast<const char*>(samples.data()), data_bytes);
    return path;
}

} // namespace

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

// Regression: play_sound() used to return a shared_ptr<ma_sound> with the
// DEFAULT deleter. Dropping the handle — the normal way to fire a one-shot —
// raw-deleted the ma_sound without calling ma_sound_uninit(), leaving a freed
// but still-registered sound in the engine's node graph. miniaudio's audio
// thread then mixed from released memory and the process crashed.
TEST(Audio, PlaySoundFireAndForgetIsSafe) {
    AudioEngine engine;
    if (!engine.initialized()) GTEST_SKIP() << "Audio not available in this env";

    const std::string path = write_temp_wav("loom2d_test_tone_ff.wav");
    for (int i = 0; i < 32; ++i)
        engine.play_sound(path, 0.05f);   // handle dropped immediately
    SUCCEED();                             // must reach here without crashing
    std::remove(path.c_str());
}

TEST(Audio, PlaySoundReturnsALiveHandle) {
    AudioEngine engine;
    if (!engine.initialized()) GTEST_SKIP() << "Audio not available in this env";

    const std::string path = write_temp_wav("loom2d_test_tone_h.wav");
    SoundHandle h = engine.play_sound(path, 0.05f);
    EXPECT_NE(h.impl, nullptr);
    EXPECT_NO_THROW(h.set_volume(0.5f));
    EXPECT_NO_THROW(h.stop());
    std::remove(path.c_str());
}

// A one-shot must never outlive its engine: the AudioEngine destructor has to
// uninit any still-playing sounds BEFORE ma_engine_uninit().
TEST(Audio, EngineDestroyedWhileSoundStillPlaying) {
    const std::string path = write_temp_wav("loom2d_test_tone_d.wav");
    {
        AudioEngine engine;
        if (!engine.initialized()) {
            std::remove(path.c_str());
            GTEST_SKIP() << "Audio not available in this env";
        }
        engine.play_sound(path, 0.05f);   // still playing as the engine dies
    }
    SUCCEED();
    std::remove(path.c_str());
}

TEST(Audio, MultipleEnginesIndependent) {
    // Two engines can coexist without interfering
    AudioEngine e1, e2;
    EXPECT_NO_THROW({
        e1.set_master_volume(0.8f);
        e2.set_master_volume(0.5f);
    });
}
