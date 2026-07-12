#include "audio/audio.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>

// miniaudio single-header implementation (exactly one TU)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace loom {

namespace {

// Wrap an *already initialised* ma_sound so that releasing the last reference
// uninits it before freeing. A plain `delete` would hand the memory back while
// the sound is still registered in the engine's node graph, and the audio thread
// would then mix from freed memory.
//
// The deleter also captures a reference to the owning engine. A caller may hold
// a SoundHandle after the AudioEngine is destroyed; keeping the ma_engine alive
// here guarantees ma_sound_uninit() never runs against a dead engine. The
// captured shared_ptr is released after the lambda body, so the engine is torn
// down only once its last sound is gone.
std::shared_ptr<ma_sound> own_sound(ma_sound* raw, std::shared_ptr<ma_engine> engine) {
    return std::shared_ptr<ma_sound>(
        raw, [engine = std::move(engine)](ma_sound* s) {
            if (s) {
                ma_sound_uninit(s);
                delete s;
            }
        });
}

} // namespace

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
    auto* raw = new ma_engine{};
    ma_result r = ma_engine_init(nullptr, raw);
    if (r != MA_SUCCESS) {
        delete raw;
        m_initialized = false;
        // Non-fatal: game can run without audio (mobile muted, headless CI, etc.)
        return;
    }
    m_engine = std::shared_ptr<ma_engine>(raw, [](ma_engine* e) {
        ma_engine_uninit(e);
        delete e;
    });
    m_initialized = true;
}

AudioEngine::~AudioEngine() {
    stop_music();
    // Release the engine's own references to its one-shots. Sounds the caller
    // still holds stay alive — and they keep the ma_engine alive with them (see
    // own_sound), so the engine is only uninit'd once the last sound is gone.
    m_active.clear();
}

void AudioEngine::reap_finished() {
    m_active.erase(
        std::remove_if(m_active.begin(), m_active.end(),
                       [](const std::shared_ptr<ma_sound>& s) {
                           // Finished naturally, or stopped early via the handle.
                           // Reaping only on at_end() would leak stopped sounds
                           // for the lifetime of the engine.
                           return ma_sound_at_end(s.get()) == MA_TRUE
                               || ma_sound_is_playing(s.get()) == MA_FALSE;
                       }),
        m_active.end());
}

SoundHandle AudioEngine::play_sound(const std::string& path, float volume) {
    if (!m_initialized) return {};

    reap_finished();

    // MA_SOUND_FLAG_DECODE: fully decode up front. SFX are short, and the
    // engine's resource manager caches the decoded data per path, so repeat
    // plays of the same file don't re-decode.
    auto* raw = new ma_sound{};
    ma_result r = ma_sound_init_from_file(m_engine.get(), path.c_str(),
                                          MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                          raw);
    if (r != MA_SUCCESS) {
        delete raw;             // never initialised: free without uninit
        return {};
    }

    auto snd = own_sound(raw, m_engine);
    ma_sound_set_volume(snd.get(), volume);
    ma_sound_start(snd.get());

    // The engine holds a reference until the sound finishes, so callers can
    // fire-and-forget and simply drop the handle.
    m_active.push_back(snd);
    return SoundHandle{snd};
}

void AudioEngine::play_music(const std::string& path, float volume, bool loop) {
    if (!m_initialized) return;
    stop_music();

    auto* raw = new ma_sound{};
    ma_result r = ma_sound_init_from_file(m_engine.get(), path.c_str(),
                                          MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                          raw);
    if (r != MA_SUCCESS) {
        delete raw;
        return;
    }

    m_music = own_sound(raw, m_engine);
    ma_sound_set_volume(m_music.get(), volume);
    ma_sound_set_looping(m_music.get(), loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(m_music.get());
}

void AudioEngine::stop_music() {
    if (m_music) {
        ma_sound_stop(m_music.get());
        m_music.reset();        // the deleter uninits
    }
}

void AudioEngine::set_music_volume(float volume) {
    if (m_music) ma_sound_set_volume(m_music.get(), volume);
}

bool AudioEngine::music_playing() const {
    return m_music && ma_sound_is_playing(m_music.get());
}

void AudioEngine::set_master_volume(float volume) {
    if (m_initialized) ma_engine_set_volume(m_engine.get(), volume);
}

} // namespace loom
