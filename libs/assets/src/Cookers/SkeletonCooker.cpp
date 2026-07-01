#include "Hockey/Assets/Cookers/SkeletonCooker.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Assets/Runtime/SkeletonLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

struct SkeletonDescriptor {
    fs::path sourceModel;
    size_t skinIndex = 0;
    std::string name;
};

Result<SkeletonDescriptor> LoadSkeletonDescriptor(const fs::path& path) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const YAML::Exception& e) {
        return Result<SkeletonDescriptor>::Fail(std::string("skeleton descriptor parse error: ") + e.what());
    }

    const YAML::Node node = root["Skeleton"];
    if (!node || !node.IsMap()) {
        return Result<SkeletonDescriptor>::Fail("skeleton descriptor missing top-level 'Skeleton' map: " +
                                                path.string());
    }
    if (!node["SourceModel"] || !node["SkinIndex"]) {
        return Result<SkeletonDescriptor>::Fail("skeleton descriptor missing SourceModel or SkinIndex: " +
                                                path.string());
    }

    SkeletonDescriptor descriptor;
    descriptor.sourceModel = node["SourceModel"].as<std::string>();
    descriptor.skinIndex = node["SkinIndex"].as<size_t>();
    if (const YAML::Node name = node["Name"]) {
        descriptor.name = name.as<std::string>();
    }
    return Result<SkeletonDescriptor>::Ok(std::move(descriptor));
}

} // namespace

CookResult SkeletonCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    Result<SkeletonDescriptor> descriptor = LoadSkeletonDescriptor(context.projectRoot / metadata.rawPath);
    if (!descriptor) {
        result.success = false;
        result.error = descriptor.error;
        return result;
    }

    const fs::path gltfAbsolute = context.projectRoot / descriptor.value.sourceModel;
    Result<GltfScene> scene = GltfLoader::Load(gltfAbsolute);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }
    if (descriptor.value.skinIndex >= scene.value.skeletons.size()) {
        result.success = false;
        result.error = "glTF skeleton index out of range: " + metadata.rawPath.generic_string();
        return result;
    }

    const GltfSkeletonData& source = scene.value.skeletons[descriptor.value.skinIndex];
    SkeletonAsset asset;
    asset.id = metadata.id;
    asset.name = descriptor.value.name.empty() ? source.name : descriptor.value.name;
    asset.bones = source.bones;

    const uint64_t sourceHash = AssetHash::HashFile(gltfAbsolute);
    const std::vector<std::byte> cooked = SkeletonLoader::EncodeCooked(asset, sourceHash);

    const fs::path cookedAbsolute = context.cookedRoot / "assets" /
                                    AssetPath::CookedSubdirectory(AssetType::Skeleton) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Skeleton));
    const Status created = FileSystem::CreateDirectories(cookedAbsolute.parent_path());
    if (!created) {
        result.success = false;
        result.error = created.error;
        return result;
    }

    std::ofstream out(cookedAbsolute, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        result.success = false;
        result.error = "cannot write cooked skeleton: " + cookedAbsolute.string();
        return result;
    }
    out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
