#include "Test.hpp"

#include "Hockey/Audio/HockeyAudioController.hpp"

#include <vector>

using namespace Hockey;

namespace {

class FakeAudioSink final : public IAudioCueSink {
public:
    void PlayCue(const AudioCue& cue) override {
        cues.push_back(cue);
    }

    std::vector<AudioCue> cues;
};

} // namespace

void RunHockeyAudioControllerTests() {
    HockeyTest::BeginSuite("HockeyAudioControllerTests");

    AudioCueMap map = MakeDefaultHockeyAudioCueMap();
    map.shot = AssetID{10};
    map.countdown = AssetID{11};
    map.goalieShield = AssetID{12};
    map.playerBoost = AssetID{13};

    FakeAudioSink sink;
    HockeyAudioController controller;
    controller.SetCueMap(map);
    controller.SetSink(&sink);

    controller.Trigger(AudioCueId::Shot, glm::vec3{1.0f, 0.0f, 2.0f});
    controller.Trigger(AudioCueId::Countdown);
    controller.Trigger(AudioCueId::GoalieShield);
    controller.Trigger(AudioCueId::PlayerBoost, glm::vec3{0.0f, 0.0f, 3.0f}, 0.8f);

    HK_CHECK_MSG(sink.cues.size() == 4, "cue ids emit audio cues");
    if (sink.cues.size() == 4) {
        HK_CHECK_MSG(sink.cues[0].clip == AssetID{10} && sink.cues[0].bus == AudioBusType::SFX, "shot maps to SFX");
        HK_CHECK_MSG(sink.cues[0].position.has_value(), "positioned cue preserves position");
        HK_CHECK_MSG(sink.cues[1].clip == AssetID{11} && sink.cues[1].bus == AudioBusType::UI, "countdown maps to UI");
        HK_CHECK_MSG(sink.cues[2].clip == AssetID{12}, "shield cue maps");
        HK_CHECK_MSG(sink.cues[3].clip == AssetID{13} && sink.cues[3].volume == 0.8f, "boost cue maps volume");
    }
}
