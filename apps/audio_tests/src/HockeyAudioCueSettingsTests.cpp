#include "Test.hpp"

#include "Hockey/Audio/AudioEvents.hpp"
#include "Hockey/Core/Config.hpp"

#include <string>

using namespace Hockey;

namespace {

const AudioCueConfigBinding* FindBinding(AudioCueId cue) {
    for (const AudioCueConfigBinding& binding : HockeyAudioCueConfigBindings()) {
        if (binding.cue == cue) {
            return &binding;
        }
    }
    return nullptr;
}

} // namespace

void RunHockeyAudioCueSettingsTests() {
    HockeyTest::BeginSuite("HockeyAudioCueSettingsTests");

    const AudioCueConfigBinding* shot = FindBinding(AudioCueId::Shot);
    const AudioCueConfigBinding* puckMetal = FindBinding(AudioCueId::PuckMetalCollision);
    HK_CHECK_MSG(shot != nullptr, "shot cue has an editable config binding");
    HK_CHECK_MSG(puckMetal != nullptr, "puck metal collision cue has an editable config binding");

    Config empty;
    if (shot != nullptr) {
        HK_CHECK_EQ(HockeyAudioCueRawPath(empty, *shot), std::string("data/raw/audio/Shot.mp3"));
    }

    Config overrideConfig;
    overrideConfig.SetString("audio_cues.shot", "data/raw/audio/CustomShot.wav");
    overrideConfig.SetString("audio_cues.puck_metal_collision", "data/raw/audio/PostHit.flac");
    if (shot != nullptr && puckMetal != nullptr) {
        HK_CHECK_EQ(HockeyAudioCueRawPath(overrideConfig, *shot), std::string("data/raw/audio/CustomShot.wav"));
        HK_CHECK_EQ(HockeyAudioCueRawPath(overrideConfig, *puckMetal), std::string("data/raw/audio/PostHit.flac"));
    }

    SaveDefaultHockeyAudioCuePaths(overrideConfig);
    if (shot != nullptr && puckMetal != nullptr) {
        HK_CHECK_EQ(HockeyAudioCueRawPath(overrideConfig, *shot), std::string("data/raw/audio/Shot.mp3"));
        HK_CHECK_EQ(HockeyAudioCueRawPath(overrideConfig, *puckMetal),
                    std::string("data/raw/audio/PuckMetalCollision.mp3"));
    }
}
