#include "Test.hpp"

#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Audio/AudioClip.hpp"

using namespace Hockey;

void RunAudioClipTests() {
    HockeyTest::BeginSuite("AudioClipTests");

    AudioAsset empty;
    empty.id = AssetID{42};
    empty.debugName = "Empty";
    empty.sourceExtension = ".mp3";
    Result<AudioClip> decoded = AudioClip::Decode(empty);
    HK_CHECK_MSG(!decoded, "empty encoded audio fails to decode");
}
