#include "Hockey/Assets/Cookers/SceneCooker.hpp"
#include "Hockey/Assets/Cookers/PrefabCooker.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Serialization/SceneDependencies.hpp"

namespace Hockey {
namespace fs = std::filesystem;

namespace {

CookResult CookYamlAsset(const CookContext& context, AssetType type) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    const fs::path rawAbsolute = context.projectRoot / metadata.rawPath;
    const Result<std::string> text = FileSystem::ReadTextFile(rawAbsolute);
    if (!text) {
        result.success = false;
        result.error = text.error;
        return result;
    }
    const Status valid = SceneDependencies::Validate(text.value);
    if (!valid) {
        result.success = false;
        result.error = valid.error;
        return result;
    }

    if (context.database != nullptr) {
        result.dependencies = SceneDependencies::Collect(text.value, *context.database);
    }

    const fs::path cookedAbsolute = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(type) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(type));
    const Status wrote = FileSystem::WriteTextFile(cookedAbsolute, text.value);
    if (!wrote) {
        result.success = false;
        result.error = wrote.error;
        return result;
    }

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace

CookResult SceneCooker::Cook(const CookContext& context) {
    return CookYamlAsset(context, AssetType::Scene);
}

CookResult PrefabCooker::Cook(const CookContext& context) {
    return CookYamlAsset(context, AssetType::Prefab);
}

} // namespace Hockey
