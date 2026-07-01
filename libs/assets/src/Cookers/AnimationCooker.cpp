#include "Hockey/Assets/Cookers/AnimationCooker.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Runtime/AnimationLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>
#include <fstream>

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

struct AnimationDescriptor {
    fs::path sourceModel;
    size_t animationIndex = 0;
    std::string name;
    fs::path skeletonPath;
};

Result<AnimationDescriptor> LoadAnimationDescriptor(const fs::path& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const YAML::Exception& e) {
        return Result<AnimationDescriptor>::Fail(std::string("animation descriptor parse error: ") + e.what());
    }

    const YAML::Node node = root["Animation"];
    if (!node || !node.IsMap()) {
        return Result<AnimationDescriptor>::Fail("animation descriptor missing top-level 'Animation' map: " +
                                                 path.string());
    }
    if (!node["SourceModel"] || !node["AnimationIndex"] || !node["Skeleton"]) {
        return Result<AnimationDescriptor>::Fail(
            "animation descriptor missing SourceModel, AnimationIndex, or Skeleton: " + path.string());
    }

    AnimationDescriptor descriptor;
    descriptor.sourceModel = node["SourceModel"].as<std::string>();
    descriptor.animationIndex = node["AnimationIndex"].as<size_t>();
    descriptor.skeletonPath = node["Skeleton"].as<std::string>();
    if (const YAML::Node name = node["Name"]) {
        descriptor.name = name.as<std::string>();
    }
    return Result<AnimationDescriptor>::Ok(std::move(descriptor));
}

} // namespace

CookResult AnimationCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    Result<AnimationDescriptor> descriptor = LoadAnimationDescriptor(context.projectRoot / metadata.rawPath);
    if (!descriptor) {
        result.success = false;
        result.error = descriptor.error;
        return result;
    }

    const AssetMetadata* skeletonMeta =
        context.database == nullptr ? nullptr : context.database->FindByRawPath(descriptor.value.skeletonPath);
    if (skeletonMeta == nullptr) {
        result.success = false;
        result.error = "animation skeleton dependency not found: " + descriptor.value.skeletonPath.generic_string();
        return result;
    }

    const fs::path gltfAbsolute = context.projectRoot / descriptor.value.sourceModel;
    Result<GltfScene> scene = GltfLoader::Load(gltfAbsolute);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }
    if (descriptor.value.animationIndex >= scene.value.animations.size()) {
        result.success = false;
        result.error = "glTF animation index out of range: " + metadata.rawPath.generic_string();
        return result;
    }

    const GltfAnimationData& source = scene.value.animations[descriptor.value.animationIndex];
    AnimationAsset asset;
    asset.id = metadata.id;
    asset.name = descriptor.value.name.empty() ? source.name : descriptor.value.name;
    asset.skeletonAsset = skeletonMeta->id;
    asset.durationSeconds = source.durationSeconds;
    asset.looping = source.looping;
    asset.tracks = source.tracks;

    const uint64_t sourceHash = AssetHash::HashFile(gltfAbsolute);
    const std::vector<std::byte> cooked = AnimationLoader::EncodeCooked(asset, sourceHash);

    const fs::path cookedAbsolute = context.cookedRoot / "assets" /
                                    AssetPath::CookedSubdirectory(AssetType::Animation) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Animation));
    const Status created = FileSystem::CreateDirectories(cookedAbsolute.parent_path());
    if (!created) {
        result.success = false;
        result.error = created.error;
        return result;
    }

    std::ofstream out(cookedAbsolute, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        result.success = false;
        result.error = "cannot write cooked animation: " + cookedAbsolute.string();
        return result;
    }
    out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));

    result.dependencies.push_back(skeletonMeta->id);
    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
