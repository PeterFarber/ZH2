#include "Hockey/Assets/Importers/GltfImporter.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"

#include <algorithm>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
void AddDependency(std::vector<AssetID>& deps, AssetID id) {
    if (id.IsValid() && std::find(deps.begin(), deps.end(), id) == deps.end()) {
        deps.push_back(id);
    }
}
} // namespace

bool GltfImporter::SupportsExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".gltf" || ext == ".glb";
}

std::string GltfImporter::MeshSubAssetPath(const fs::path& modelRawPath, size_t meshIndex) {
    return modelRawPath.generic_string() + "#mesh" + std::to_string(meshIndex);
}

std::string GltfImporter::MaterialSubAssetPath(const fs::path& modelRawPath, size_t materialIndex) {
    return modelRawPath.generic_string() + "#material" + std::to_string(materialIndex);
}

bool GltfImporter::IsSubAsset(const fs::path& rawPath) {
    return rawPath.generic_string().find('#') != std::string::npos;
}

bool GltfImporter::ParseSubAsset(const fs::path& rawPath, fs::path& outModelRawPath, size_t& outIndex,
                                 bool& outIsMesh) {
    const std::string s = rawPath.generic_string();
    const size_t hash = s.find('#');
    if (hash == std::string::npos) {
        return false;
    }
    outModelRawPath = s.substr(0, hash);
    const std::string token = s.substr(hash + 1);

    std::string prefix;
    if (token.rfind("mesh", 0) == 0) {
        outIsMesh = true;
        prefix = "mesh";
    } else if (token.rfind("material", 0) == 0) {
        outIsMesh = false;
        prefix = "material";
    } else {
        return false;
    }
    try {
        outIndex = static_cast<size_t>(std::stoul(token.substr(prefix.size())));
    } catch (...) {
        return false;
    }
    return true;
}

fs::path GltfImporter::TextureProjectPath(const fs::path& projectRoot, const fs::path& modelRawPath,
                                          const std::string& uri) {
    if (uri.empty()) {
        return {};
    }
    const fs::path modelAbsolute = projectRoot / modelRawPath;
    const fs::path textureAbsolute = modelAbsolute.parent_path() / uri;
    return AssetPath::ToProjectRelative(projectRoot, textureAbsolute);
}

ImportResult GltfImporter::Import(const ImportContext& context) {
    ImportResult result;

    Result<GltfScene> scene = GltfLoader::Load(context.rawPath);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }

    const fs::path modelRawPath = AssetPath::ToProjectRelative(context.projectRoot, context.rawPath);

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Model;
    result.metadata.name = context.rawPath.stem().string();

    AssetDatabase* db = context.database;
    std::vector<AssetID> modelDeps;

    // Generated material sub-assets (and their texture dependencies).
    std::vector<AssetID> materialIds(scene.value.materials.size());
    for (size_t i = 0; i < scene.value.materials.size(); ++i) {
        const GltfMaterialData& material = scene.value.materials[i];
        const std::string subPath = MaterialSubAssetPath(modelRawPath, i);

        AssetMetadata meta;
        meta.type = AssetType::Material;
        meta.rawPath = subPath;
        meta.name = material.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Material);
            for (const std::string* uri :
                 {&material.baseColorTexture, &material.normalTexture, &material.metallicRoughnessTexture,
                  &material.occlusionTexture, &material.emissiveTexture}) {
                const fs::path texRel = TextureProjectPath(context.projectRoot, modelRawPath, *uri);
                if (!texRel.empty()) {
                    AddDependency(meta.dependencies, db->GetOrCreateIDForPath(texRel, AssetType::Texture));
                }
            }
        }
        materialIds[i] = meta.id;
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    // Generated mesh sub-assets (depend on the materials their submeshes use).
    for (size_t i = 0; i < scene.value.meshes.size(); ++i) {
        const GltfMeshData& mesh = scene.value.meshes[i];
        const std::string subPath = MeshSubAssetPath(modelRawPath, i);

        AssetMetadata meta;
        meta.type = AssetType::Mesh;
        meta.rawPath = subPath;
        meta.name = mesh.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Mesh);
            for (const int materialIndex : mesh.submeshMaterialIndex) {
                if (materialIndex >= 0 && static_cast<size_t>(materialIndex) < materialIds.size()) {
                    AddDependency(meta.dependencies, materialIds[materialIndex]);
                }
            }
        }
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    result.metadata.dependencies = modelDeps;
    return result;
}

} // namespace Hockey
