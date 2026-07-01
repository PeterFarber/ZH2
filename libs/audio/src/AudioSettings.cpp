#include "Hockey/Audio/AudioSettings.hpp"

#include "Hockey/Core/Config.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace Hockey {

namespace {

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::uint32_t PositiveUInt(int value, std::uint32_t fallback) {
    return value > 0 ? static_cast<std::uint32_t>(value) : fallback;
}

} // namespace

float NormalizeAudioVolume(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

AudioBackendMode AudioBackendModeFromString(const char* text) {
    const std::string value = text != nullptr ? Lower(text) : std::string{};
    if (value == "null") {
        return AudioBackendMode::Null;
    }
    return AudioBackendMode::Auto;
}

const char* AudioBackendModeToString(AudioBackendMode mode) {
    switch (mode) {
    case AudioBackendMode::Null:
        return "Null";
    case AudioBackendMode::Auto:
    default:
        return "Auto";
    }
}

AudioSettings MakeDefaultAudioSettings() {
    return AudioSettings{};
}

AudioSettings LoadAudioSettings(const Config& config) {
    AudioSettings settings = MakeDefaultAudioSettings();
    settings.enabled = config.GetBool("audio.enabled", settings.enabled);
    settings.backend = AudioBackendModeFromString(config.GetString("audio.backend", AudioBackendModeToString(settings.backend)).c_str());
    settings.masterVolume = NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.master_volume", settings.masterVolume)));
    settings.musicVolume = NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.music_volume", settings.musicVolume)));
    settings.sfxVolume = NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.sfx_volume", settings.sfxVolume)));
    settings.ambienceVolume =
        NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.ambience_volume", settings.ambienceVolume)));
    settings.uiVolume = NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.ui_volume", settings.uiVolume)));
    settings.crowdVolume =
        NormalizeAudioVolume(static_cast<float>(config.GetDouble("audio.crowd_volume", settings.crowdVolume)));
    settings.muteWhenUnfocused = config.GetBool("audio.mute_when_unfocused", settings.muteWhenUnfocused);
    settings.enableSpatialAudio = config.GetBool("audio.enable_spatial_audio", settings.enableSpatialAudio);
    settings.sampleRate = PositiveUInt(config.GetInt("audio.sample_rate", static_cast<int>(settings.sampleRate)), settings.sampleRate);
    settings.channels = PositiveUInt(config.GetInt("audio.channels", static_cast<int>(settings.channels)), settings.channels);
    return settings;
}

void SaveAudioSettings(Config& config, const AudioSettings& settings) {
    config.SetBool("audio.enabled", settings.enabled);
    config.SetString("audio.backend", AudioBackendModeToString(settings.backend));
    config.SetDouble("audio.master_volume", NormalizeAudioVolume(settings.masterVolume));
    config.SetDouble("audio.music_volume", NormalizeAudioVolume(settings.musicVolume));
    config.SetDouble("audio.sfx_volume", NormalizeAudioVolume(settings.sfxVolume));
    config.SetDouble("audio.ambience_volume", NormalizeAudioVolume(settings.ambienceVolume));
    config.SetDouble("audio.ui_volume", NormalizeAudioVolume(settings.uiVolume));
    config.SetDouble("audio.crowd_volume", NormalizeAudioVolume(settings.crowdVolume));
    config.SetBool("audio.mute_when_unfocused", settings.muteWhenUnfocused);
    config.SetBool("audio.enable_spatial_audio", settings.enableSpatialAudio);
    config.SetInt("audio.sample_rate", static_cast<int>(settings.sampleRate));
    config.SetInt("audio.channels", static_cast<int>(settings.channels));
}

} // namespace Hockey
