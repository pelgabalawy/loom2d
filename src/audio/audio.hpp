#pragma once
#include <string>
#include <unordered_map>
#include <memory>

// Forward-declare miniaudio types to keep the header clean
typedef struct ma_engine ma_engine;
typedef struct ma_sound  ma_sound;

namespace loom {

// One-shot sound effect handle (fire-and-forget)
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
    ma_engine*               m_engine      = nullptr;
    std::shared_ptr<ma_sound> m_music;
    bool                     m_initialized = false;
};

} // namespace loom
