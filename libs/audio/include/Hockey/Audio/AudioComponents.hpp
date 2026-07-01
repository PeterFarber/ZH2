#pragma once

#include <cstdint>

#include "Hockey/Audio/AudioBus.hpp"
#include "Hockey/ECS/ComponentMetadata.hpp"

namespace Hockey {

struct AudioListenerComponent {
    bool primary = true;
};

struct AudioSourceComponent {
    std::uint64_t clipAsset = 0;
    AudioBusType bus = AudioBusType::SFX;
    bool playOnStart = false;
    bool loop = false;
    bool spatial = true;
    float volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 50.0f;
};

void RegisterAudioComponents();

template <> struct ComponentTraits<AudioListenerComponent> {
    static constexpr const char* Name = "AudioListenerComponent";
};

template <> struct ComponentTraits<AudioSourceComponent> {
    static constexpr const char* Name = "AudioSourceComponent";
};

} // namespace Hockey
