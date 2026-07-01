#include "Test.hpp"

#include "Hockey/Audio/AudioSystem.hpp"

using namespace Hockey;

void RunAudioSystemTests() {
    HockeyTest::BeginSuite("AudioSystemTests");

    AudioSettings settings = MakeDefaultAudioSettings();
    settings.backend = AudioBackendMode::Null;
    AudioSystem audio(settings);
    HK_CHECK_MSG(static_cast<bool>(audio.Initialize(nullptr)), "null audio system initializes without asset manager");
    HK_CHECK_MSG(audio.IsInitialized(), "audio system reports initialized");
    audio.PlayOneShot(AssetID{123}, AudioBusType::SFX, 0.75f);
    HK_CHECK_MSG(audio.DebugQueuedCommandCount() == 1, "null backend records queued playback command");
    audio.Shutdown();
    HK_CHECK_MSG(!audio.IsInitialized(), "audio system shuts down");
}
