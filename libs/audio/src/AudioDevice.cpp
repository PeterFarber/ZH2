#include "Hockey/Audio/AudioDevice.hpp"

#include "Hockey/Audio/AudioClip.hpp"
#include "Hockey/Audio/AudioMixer.hpp"

#include <miniaudio.h>

#include <vector>

namespace Hockey {

namespace {

struct ActiveSound {
    std::shared_ptr<AudioClip> clip;
    ma_audio_buffer buffer{};
    ma_sound sound{};
    bool bufferInitialized = false;
    bool soundInitialized = false;

    ~ActiveSound() {
        if (soundInitialized) {
            ma_sound_uninit(&sound);
        }
        if (bufferInitialized) {
            ma_audio_buffer_uninit(&buffer);
        }
    }
};

} // namespace

struct AudioDevice::Impl {
    AudioSettings settings;
    bool initialized = false;
    bool nullBackend = true;
    ma_engine engine{};
    std::vector<std::unique_ptr<ActiveSound>> activeSounds;
};

AudioDevice::AudioDevice() = default;
AudioDevice::~AudioDevice() = default;

Status AudioDevice::Init(const AudioSettings& settings) {
    Shutdown();
    m_Impl = std::make_unique<Impl>();
    m_Impl->settings = settings;

    if (!settings.enabled || settings.backend == AudioBackendMode::Null) {
        m_Impl->initialized = true;
        m_Impl->nullBackend = true;
        return Status::Ok();
    }

    ma_engine_config config = ma_engine_config_init();
    config.sampleRate = settings.sampleRate;
    config.channels = settings.channels;
    const ma_result result = ma_engine_init(&config, &m_Impl->engine);
    if (result != MA_SUCCESS) {
        m_Impl.reset();
        return Status::Fail("failed to initialize audio engine");
    }

    m_Impl->initialized = true;
    m_Impl->nullBackend = false;
    return Status::Ok();
}

void AudioDevice::Shutdown() {
    if (!m_Impl) {
        return;
    }
    StopAll();
    if (m_Impl->initialized && !m_Impl->nullBackend) {
        ma_engine_uninit(&m_Impl->engine);
    }
    m_Impl.reset();
}

void AudioDevice::Update() {
    if (!m_Impl || m_Impl->nullBackend) {
        return;
    }
    auto& sounds = m_Impl->activeSounds;
    for (auto it = sounds.begin(); it != sounds.end();) {
        ActiveSound* sound = it->get();
        if (sound == nullptr || !sound->soundInitialized || !ma_sound_is_playing(&sound->sound)) {
            it = sounds.erase(it);
        } else {
            ++it;
        }
    }
}

bool AudioDevice::IsRunning() const {
    return m_Impl != nullptr && m_Impl->initialized;
}

bool AudioDevice::IsNullBackend() const {
    return m_Impl == nullptr || m_Impl->nullBackend;
}

Status AudioDevice::PlayOneShot(std::shared_ptr<AudioClip> clip, AudioBusType bus, float volume, const AudioMixer& mixer) {
    if (!m_Impl || !m_Impl->initialized) {
        return Status::Fail("audio device is not initialized");
    }
    if (m_Impl->nullBackend) {
        return Status::Ok();
    }
    if (!clip || clip->samples.empty() || clip->channels == 0 || clip->frameCount == 0) {
        return Status::Fail("audio clip is empty");
    }

    auto active = std::make_unique<ActiveSound>();
    active->clip = std::move(clip);

    ma_audio_buffer_config bufferConfig =
        ma_audio_buffer_config_init(ma_format_f32, active->clip->channels, active->clip->frameCount,
                                    active->clip->samples.data(), nullptr);
    ma_result result = ma_audio_buffer_init(&bufferConfig, &active->buffer);
    if (result != MA_SUCCESS) {
        return Status::Fail("failed to initialize audio buffer");
    }
    active->bufferInitialized = true;

    result = ma_sound_init_from_data_source(&m_Impl->engine, &active->buffer, 0, nullptr, &active->sound);
    if (result != MA_SUCCESS) {
        return Status::Fail("failed to initialize audio sound");
    }
    active->soundInitialized = true;
    ma_sound_set_volume(&active->sound, mixer.EffectiveVolume(bus) * NormalizeAudioVolume(volume));
    result = ma_sound_start(&active->sound);
    if (result != MA_SUCCESS) {
        return Status::Fail("failed to start audio sound");
    }
    m_Impl->activeSounds.push_back(std::move(active));
    return Status::Ok();
}

void AudioDevice::StopAll() {
    if (!m_Impl) {
        return;
    }
    m_Impl->activeSounds.clear();
}

} // namespace Hockey
