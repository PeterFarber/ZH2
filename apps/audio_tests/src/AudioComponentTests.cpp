#include "Test.hpp"

#include "Hockey/Audio/AudioComponents.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"

#include <filesystem>
#include <string>

using namespace Hockey;

namespace {

bool HasField(const ComponentMetadata* metadata, const char* name) {
    if (metadata == nullptr) {
        return false;
    }
    for (const FieldMetadata& field : metadata->fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunAudioComponentTests() {
    HockeyTest::BeginSuite("AudioComponentTests");

    RegisterAudioComponents();

    const ComponentMetadata* sourceMeta = ComponentRegistry::Get().FindByName("AudioSourceComponent");
    HK_CHECK_MSG(sourceMeta != nullptr, "audio source component metadata registered");
    HK_CHECK_MSG(HasField(sourceMeta, "ClipAsset"), "clip asset field exists");
    HK_CHECK_MSG(HasField(sourceMeta, "Bus"), "bus field exists");

    Scene scene("AudioScene");
    Entity entity = scene.CreateEntity("Horn");
    AudioSourceComponent& source = entity.AddComponent<AudioSourceComponent>();
    source.clipAsset = 99;
    source.bus = AudioBusType::SFX;
    source.playOnStart = true;
    source.loop = false;
    source.spatial = true;
    source.volume = 0.75f;

    SceneSerializer serializer(scene);
    const std::filesystem::path scenePath = Paths::TempFile("audio_component_scene.yaml");
    HK_CHECK_MSG(static_cast<bool>(serializer.Serialize(scenePath)), "audio scene serializes");
    const Result<std::string> yamlText = FileSystem::ReadTextFile(scenePath);
    HK_CHECK_MSG(static_cast<bool>(yamlText), "audio scene yaml reads");
    if (yamlText) {
        HK_CHECK_MSG(yamlText.value.find("AudioSourceComponent") != std::string::npos, "audio source serializes");
        HK_CHECK_MSG(yamlText.value.find("ClipAsset: 99") != std::string::npos, "clip asset id serializes");
        HK_CHECK_MSG(yamlText.value.find("Bus: SFX") != std::string::npos, "audio bus serializes");
    }

    FileSystem::Remove(scenePath);
}
