#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Forward-declare miniaudio types to keep the header clean
typedef struct ma_engine ma_engine;
typedef struct ma_sound  ma_sound;

namespace loom {

// One-shot sound effect handle (fire-and-forget).
//
// The handle is optional: AudioEngine keeps every one-shot alive until it
// finishes on its own, so `audio.play_sound(path)` and discarding the result is
// safe. `impl` owns the ma_sound through a deleter that calls ma_sound_uninit()
// — never free a ma_sound without uninit'ing it first, or miniaudio's audio
// thread will keep mixing from released memory.
struct SoundHandle {
    bool playing() const;
    void stop();
    void set_volume(float v);  // 0..1

    std::shared_ptr<ma_sound> impl;
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool initialized() const { return m_initialized; }

    // Play a one-shot sound effect (multiple overlapping instances OK)
    SoundHandle play_sound(const std::string& path, float volume = 1.f);

    // Streaming background music — only one BGM at a time
    void play_music(const std::string& path, float volume = 1.f,
                    bool loop = true);
    void stop_music();
    void set_music_volume(float volume); // 0..1
    bool music_playing() const;

    void set_master_volume(float volume); // 0..1

private:
    // Drop any one-shots that have finished (or were stopped early).
    void reap_finished();

    // The engine is refcounted and every sound's deleter holds a reference, so
    // a ma_engine can never be uninit'd while one of its ma_sounds is still
    // alive. That matters because a caller may keep a SoundHandle after the
    // AudioEngine has gone away — without this, the handle's deleter would run
    // ma_sound_uninit() against a destroyed engine.
    std::shared_ptr<ma_engine>             m_engine;
    std::shared_ptr<ma_sound>              m_music;
    std::vector<std::shared_ptr<ma_sound>> m_active;   // keeps one-shots alive
    bool                                   m_initialized = false;
};

} // namespace loom
