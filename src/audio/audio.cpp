#include "audio/audio.hpp"
#include <stdexcept>
#include <cstring>

// miniaudio single-header implementation (exactly one TU)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace loom {

// ── SoundHandle ──────────────────────────────────────────────────────────────

bool SoundHandle::playing() const {
    return impl && ma_sound_is_playing(impl.get());
}

void SoundHandle::stop() {
    if (impl) ma_sound_stop(impl.get());
}

void SoundHandle::set_volume(float v) {
    if (impl) ma_sound_set_volume(impl.get(), v);
}

// ── AudioEngine ──────────────────────────────────────────────────────────────

AudioEngine::AudioEngine() {
    m_engine = new ma_engine{};
    ma_result r = ma_engine_init(nullptr, m_engine);
    if (r != MA_SUCCESS) {
        delete m_engine;
        m_engine      = nullptr;
        m_initialized = false;
        // Non-fatal: game can run without audio (mobile muted, headless CI, etc.)
        return;
    }
    m_initialized = true;
}

AudioEngine::~AudioEngine() {
    stop_music();
    if (m_initialized) ma_engine_uninit(m_engine);
    delete m_engine;
}

SoundHandle AudioEngine::play_sound(const std::string& path, float volume) {
    if (!m_initialized) return {};

    auto snd = std::make_shared<ma_sound>();
    ma_result r = ma_sound_init_from_file(m_engine, path.c_str(),
                                          MA_SOUND_FLAG_ASYNC, nullptr, nullptr,
                                          snd.get());
    if (r != MA_SUCCESS) return {};

    ma_sound_set_volume(snd.get(), volume);
    ma_sound_start(snd.get());

    // Self-deleting on natural completion
    return SoundHandle{snd};
}

void AudioEngine::play_music(const std::string& path, float volume, bool loop) {
    if (!m_initialized) return;
    stop_music();

    m_music = std::make_shared<ma_sound>();
    ma_result r = ma_sound_init_from_file(m_engine, path.c_str(),
                                          MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                          m_music.get());
    if (r != MA_SUCCESS) { m_music.reset(); return; }

    ma_sound_set_volume(m_music.get(), volume);
    ma_sound_set_looping(m_music.get(), loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(m_music.get());
}

void AudioEngine::stop_music() {
    if (m_music) {
        ma_sound_stop(m_music.get());
        ma_sound_uninit(m_music.get());
        m_music.reset();
    }
}

void AudioEngine::set_music_volume(float volume) {
    if (m_music) ma_sound_set_volume(m_music.get(), volume);
}

bool AudioEngine::music_playing() const {
    return m_music && ma_sound_is_playing(m_music.get());
}

void AudioEngine::set_master_volume(float volume) {
    if (m_initialized) ma_engine_set_volume(m_engine, volume);
}

} // namespace loom
