#pragma once

#include <cstdint>

namespace Hockey {

class Config;

enum class AudioBackendMode {
    Auto,
    Null,
};

struct AudioSettings {
    bool enabled = true;
    AudioBackendMode backend = AudioBackendMode::Auto;
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.9f;
    float ambienceVolume = 0.8f;
    float uiVolume = 0.8f;
    float crowdVolume = 0.8f;
    bool muteWhenUnfocused = false;
    bool enableSpatialAudio = true;
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 2;
};

float NormalizeAudioVolume(float value);
AudioBackendMode AudioBackendModeFromString(const char* text);
const char* AudioBackendModeToString(AudioBackendMode mode);
AudioSettings MakeDefaultAudioSettings();
AudioSettings LoadAudioSettings(const Config& config);
void SaveAudioSettings(Config& config, const AudioSettings& settings);

} // namespace Hockey
