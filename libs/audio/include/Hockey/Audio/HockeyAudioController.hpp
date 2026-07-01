#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "Hockey/Audio/AudioEvents.hpp"

namespace Hockey {

class HockeyAudioController {
public:
    void SetCueMap(AudioCueMap map);
    const AudioCueMap& CueMap() const;
    void SetSink(IAudioCueSink* sink);

    void Trigger(AudioCueId cue, std::optional<glm::vec3> position = std::nullopt, float volumeScale = 1.0f);

private:
    AudioCueMap m_CueMap;
    IAudioCueSink* m_Sink = nullptr;
};

} // namespace Hockey
