#include "Test.hpp"

#include "Hockey/Audio/AudioMixer.hpp"

using namespace Hockey;

void RunAudioMixerTests() {
    HockeyTest::BeginSuite("AudioMixerTests");

    AudioMixer mixer;
    AudioSettings settings = MakeDefaultAudioSettings();
    settings.masterVolume = 0.5f;
    settings.sfxVolume = 0.8f;
    settings.musicVolume = 0.25f;
    mixer.ApplySettings(settings);

    HK_CHECK_NEAR(mixer.EffectiveVolume(AudioBusType::SFX), 0.4f, 1e-6f);
    HK_CHECK_NEAR(mixer.EffectiveVolume(AudioBusType::Music), 0.125f, 1e-6f);

    mixer.SetMuted(AudioBusType::SFX, true);
    HK_CHECK_NEAR(mixer.EffectiveVolume(AudioBusType::SFX), 0.0f, 1e-6f);
    HK_CHECK_NEAR(mixer.EffectiveVolume(AudioBusType::UI), 0.4f, 1e-6f);
}
