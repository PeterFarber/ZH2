#include "Test.hpp"

#include "Hockey/Audio/AudioSettings.hpp"
#include "Hockey/Core/Config.hpp"

using namespace Hockey;

void RunAudioSettingsTests() {
    HockeyTest::BeginSuite("AudioSettingsTests");

    const AudioSettings defaults = MakeDefaultAudioSettings();
    HK_CHECK_MSG(defaults.enabled, "audio enabled by default");
    HK_CHECK_MSG(defaults.backend == AudioBackendMode::Auto, "auto backend by default");
    HK_CHECK_NEAR(defaults.masterVolume, 1.0f, 1e-6f);
    HK_CHECK_NEAR(NormalizeAudioVolume(-1.0f), 0.0f, 1e-6f);
    HK_CHECK_NEAR(NormalizeAudioVolume(2.0f), 1.0f, 1e-6f);

    Config config;
    config.SetBool("audio.enabled", false);
    config.SetString("audio.backend", "Null");
    config.SetDouble("audio.master_volume", 0.25);
    config.SetDouble("audio.sfx_volume", 1.75);
    config.SetBool("audio.enable_spatial_audio", false);

    const AudioSettings loaded = LoadAudioSettings(config);
    HK_CHECK_MSG(!loaded.enabled, "enabled setting loads");
    HK_CHECK_MSG(loaded.backend == AudioBackendMode::Null, "null backend loads");
    HK_CHECK_NEAR(loaded.masterVolume, 0.25f, 1e-6f);
    HK_CHECK_NEAR(loaded.sfxVolume, 1.0f, 1e-6f);
    HK_CHECK_MSG(!loaded.enableSpatialAudio, "spatial toggle loads");
}
