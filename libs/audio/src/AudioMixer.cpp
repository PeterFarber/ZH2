#include "Hockey/Audio/AudioMixer.hpp"

namespace Hockey {

int AudioMixer::Index(AudioBusType bus) {
    return static_cast<int>(bus);
}

void AudioMixer::ApplySettings(const AudioSettings& settings) {
    SetVolume(AudioBusType::Master, settings.masterVolume);
    SetVolume(AudioBusType::Music, settings.musicVolume);
    SetVolume(AudioBusType::SFX, settings.sfxVolume);
    SetVolume(AudioBusType::Ambience, settings.ambienceVolume);
    SetVolume(AudioBusType::UI, settings.uiVolume);
    SetVolume(AudioBusType::Crowd, settings.crowdVolume);
}

void AudioMixer::SetVolume(AudioBusType bus, float volume) {
    m_Volumes[Index(bus)] = NormalizeAudioVolume(volume);
}

void AudioMixer::SetMuted(AudioBusType bus, bool muted) {
    m_Muted[Index(bus)] = muted;
}

float AudioMixer::Volume(AudioBusType bus) const {
    return m_Volumes[Index(bus)];
}

bool AudioMixer::Muted(AudioBusType bus) const {
    return m_Muted[Index(bus)];
}

float AudioMixer::EffectiveVolume(AudioBusType bus) const {
    if (Muted(AudioBusType::Master)) {
        return 0.0f;
    }
    if (bus == AudioBusType::Master) {
        return Muted(bus) ? 0.0f : Volume(bus);
    }
    if (Muted(bus)) {
        return 0.0f;
    }
    return Volume(AudioBusType::Master) * Volume(bus);
}

} // namespace Hockey
