#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Assets/Runtime/AudioLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <filesystem>
#include <string>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

std::string FakeAudioBytes() {
    return std::string{"RIFF"} + std::string(32, '\x42');
}

} // namespace

void RunAudioAssetTests() {
    HockeyTest::BeginSuite("AudioAssetTests");

    HK_CHECK_EQ(AssetManager::ClassifyExtension(".mp3"), AssetType::Audio);
    HK_CHECK_EQ(AssetManager::ClassifyExtension(".wav"), AssetType::Audio);
    HK_CHECK_EQ(AssetManager::ClassifyExtension(".flac"), AssetType::Audio);
    HK_CHECK_EQ(AssetManager::ClassifyExtension(".aup3"), AssetType::Unknown);

    const fs::path workspace = Paths::TempFile("audio_asset_ws");
    FileSystem::Remove(workspace);
    const fs::path audioDir = workspace / "data" / "raw" / "audio";
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(audioDir / "goal_horn.wav", FakeAudioBytes())),
                 "writes raw wav fixture");
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(audioDir / "menu_click.mp3", FakeAudioBytes())),
                 "writes raw mp3 fixture");
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(audioDir / "session.aup3", "audacity project")),
                 "writes unsupported audio project fixture");

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");

    AssetMetadata* wavMeta = manager.Database().FindByRawPath("data/raw/audio/goal_horn.wav");
    AssetMetadata* mp3Meta = manager.Database().FindByRawPath("data/raw/audio/menu_click.mp3");
    AssetMetadata* projectMeta = manager.Database().FindByRawPath("data/raw/audio/session.aup3");
    HK_CHECK_MSG(wavMeta != nullptr && wavMeta->type == AssetType::Audio, "wav imported as audio");
    HK_CHECK_MSG(mp3Meta != nullptr && mp3Meta->type == AssetType::Audio, "mp3 imported as audio");
    HK_CHECK_MSG(projectMeta == nullptr, "unsupported audio project is not imported");

    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");
    wavMeta = manager.Database().FindByRawPath("data/raw/audio/goal_horn.wav");
    HK_CHECK_MSG(wavMeta != nullptr && wavMeta->cooked, "wav cooked");
    HK_CHECK_MSG(wavMeta != nullptr && wavMeta->cookedPath.extension() == ".bin", "audio cooked to binary payload");
    HK_CHECK_MSG(wavMeta != nullptr && FileSystem::Exists(workspace / wavMeta->cookedPath), "cooked audio exists");

    if (wavMeta != nullptr) {
        const Result<std::shared_ptr<AudioAsset>> loaded = manager.Load<AudioAsset>(wavMeta->id);
        HK_CHECK_MSG(static_cast<bool>(loaded), "load cooked audio asset");
        if (loaded) {
            HK_CHECK_EQ(loaded.value->id, wavMeta->id);
            HK_CHECK_EQ(loaded.value->sourceExtension, std::string(".wav"));
            HK_CHECK_EQ(loaded.value->encodedBytes.size(), FakeAudioBytes().size());
            HK_CHECK(!loaded.value->debugName.empty());
        }
    }

    const std::byte malformed[]{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    const Result<AudioAsset> malformedResult = AudioLoader::DecodeCooked(malformed, sizeof(malformed), AssetID{77});
    HK_CHECK_MSG(!malformedResult, "malformed cooked audio fails to decode");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
