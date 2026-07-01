#pragma once

#include <array>

#include "Hockey/Audio/AudioBus.hpp"
#include "Hockey/Audio/AudioSettings.hpp"

namespace Hockey {

class AudioMixer {
public:
    void ApplySettings(const AudioSettings& settings);
    void SetVolume(AudioBusType bus, float volume);
    void SetMuted(AudioBusType bus, bool muted);
    float Volume(AudioBusType bus) const;
    bool Muted(AudioBusType bus) const;
    float EffectiveVolume(AudioBusType bus) const;

private:
    static constexpr int kBusCount = 6;
    static int Index(AudioBusType bus);

    std::array<float, kBusCount> m_Volumes{1.0f, 0.7f, 0.9f, 0.8f, 0.8f, 0.8f};
    std::array<bool, kBusCount> m_Muted{false, false, false, false, false, false};
};

} // namespace Hockey
