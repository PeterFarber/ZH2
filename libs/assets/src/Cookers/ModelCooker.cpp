#include "Hockey/Assets/Cookers/ModelCooker.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Assets/Importers/GltfImporter.hpp"
#include "Hockey/Assets/Runtime/ModelLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <fstream>
#include <vector>

namespace Hockey {
namespace fs = std::filesystem;

CookResult ModelCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    const fs::path gltfAbsolute = context.projectRoot / metadata.rawPath;
    Result<GltfScene> scene = GltfLoader::Load(gltfAbsolute);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }

    ModelAsset asset;
    asset.id = metadata.id;
    asset.name = metadata.name;

    if (context.database != nullptr) {
        std::vector<std::string> meshNames;
        meshNames.reserve(scene.value.meshes.size());
        for (const GltfMeshData& mesh : scene.value.meshes) {
            meshNames.push_back(mesh.name);
        }
        const std::vector<fs::path> meshPaths = GltfImporter::MeshAssetPaths(metadata.rawPath, meshNames);

        for (size_t i = 0; i < scene.value.meshes.size(); ++i) {
            const fs::path legacyPath = GltfImporter::MeshSubAssetPath(metadata.rawPath, i);
            const AssetMetadata* sub = context.database->FindByRawPath(meshPaths[i]);
            if (sub == nullptr) {
                sub = context.database->FindByRawPath(legacyPath);
            }
            if (sub != nullptr) {
                asset.meshes.push_back(sub->id);
                result.dependencies.push_back(sub->id);
            }
        }

        std::vector<std::string> materialNames;
        materialNames.reserve(scene.value.materials.size());
        for (const GltfMaterialData& material : scene.value.materials) {
            materialNames.push_back(material.name);
        }
        const std::vector<fs::path> materialPaths = GltfImporter::MaterialAssetPaths(metadata.rawPath, materialNames);

        for (size_t i = 0; i < scene.value.materials.size(); ++i) {
            const fs::path legacyPath = GltfImporter::MaterialSubAssetPath(metadata.rawPath, i);
            const AssetMetadata* sub = context.database->FindByRawPath(materialPaths[i]);
            if (sub == nullptr) {
                sub = context.database->FindByRawPath(legacyPath);
            }
            if (sub != nullptr) {
                asset.materials.push_back(sub->id);
                result.dependencies.push_back(sub->id);
            }
        }
    }

    const fs::path cookedAbsolute = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(AssetType::Model) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Model));

    const Status created = FileSystem::CreateDirectories(cookedAbsolute.parent_path());
    if (!created) {
        result.success = false;
        result.error = created.error;
        return result;
    }

    const uint64_t sourceHash = AssetHash::HashFile(gltfAbsolute);
    const std::vector<std::byte> cooked = ModelLoader::Encode(asset, sourceHash);
    std::ofstream out(cookedAbsolute, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        result.success = false;
        result.error = "cannot write cooked model: " + cookedAbsolute.string();
        return result;
    }
    out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
