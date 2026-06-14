#include "Hockey/Assets/Importers/SceneImporter.hpp"
#include "Hockey/Assets/Importers/PrefabImporter.hpp"

#include "Hockey/Core/FileSystem.hpp"
#include "Serialization/SceneDependencies.hpp"

namespace Hockey {
namespace {

ImportResult ImportYamlAsset(const ImportContext& context, AssetType type) {
    ImportResult result;
    const Result<std::string> text = FileSystem::ReadTextFile(context.rawPath);
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

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = type;
    result.metadata.name = context.rawPath.stem().string();
    if (context.database != nullptr) {
        result.metadata.dependencies = SceneDependencies::Collect(text.value, *context.database);
    }
    return result;
}

} // namespace

bool SceneImporter::SupportsExtension(const std::string&) const {
    // Selected by asset type (.scene.yaml shares the ".yaml" extension).
    return false;
}

ImportResult SceneImporter::Import(const ImportContext& context) {
    return ImportYamlAsset(context, AssetType::Scene);
}

bool PrefabImporter::SupportsExtension(const std::string&) const {
    return false;
}

ImportResult PrefabImporter::Import(const ImportContext& context) {
    return ImportYamlAsset(context, AssetType::Prefab);
}

} // namespace Hockey
