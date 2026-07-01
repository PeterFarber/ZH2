#include "Hockey/Audio/HockeyAudioController.hpp"

#include <algorithm>

namespace Hockey {

void HockeyAudioController::SetCueMap(AudioCueMap map) {
    m_CueMap = map;
}

const AudioCueMap& HockeyAudioController::CueMap() const {
    return m_CueMap;
}

void HockeyAudioController::SetSink(IAudioCueSink* sink) {
    m_Sink = sink;
}

void HockeyAudioController::Trigger(AudioCueId cueId, std::optional<glm::vec3> position, float volumeScale) {
    if (m_Sink == nullptr) {
        return;
    }
    const AssetID clip = ClipForCue(m_CueMap, cueId);
    if (!clip.IsValid()) {
        return;
    }
    AudioCue cue;
    cue.clip = clip;
    cue.bus = BusForCue(cueId);
    cue.volume = std::clamp(volumeScale, 0.0f, 1.0f);
    cue.position = position;
    m_Sink->PlayCue(cue);
}

} // namespace Hockey
