#include "Hockey/Assets/Cookers/MaterialCooker.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Importers/GltfImporter.hpp"
#include "Hockey/Assets/Importers/MaterialImporter.hpp"
#include "Hockey/Assets/Runtime/MaterialLoader.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

// Converts the authored MaterialSource (texture paths) into a MaterialSource the
// shared resolver understands, so a glTF-generated material reuses the exact
// same texture-id resolution path as an authored .material.yaml.
MaterialSource SourceFromGltf(const GltfMaterialData& gltf, const fs::path& projectRoot, const fs::path& modelRawPath) {
    MaterialSource source;
    source.name = gltf.name;
    source.baseColor = gltf.baseColor;
    source.metallic = gltf.metallic;
    source.roughness = gltf.roughness;
    source.normalStrength = gltf.normalStrength;
    source.occlusionStrength = gltf.occlusionStrength;
    source.emissiveColor = gltf.emissiveColor;
    source.emissiveStrength = gltf.emissiveStrength;
    source.alphaMode = gltf.alphaMode;
    source.alphaCutoff = gltf.alphaCutoff;

    auto toRel = [&](const std::string& uri) -> std::string {
        const fs::path rel = GltfImporter::TextureProjectPath(projectRoot, modelRawPath, uri);
        return rel.empty() ? std::string{} : rel.generic_string();
    };
    source.baseColorTexture = toRel(gltf.baseColorTexture);
    source.normalTexture = toRel(gltf.normalTexture);
    source.metallicRoughnessTexture = toRel(gltf.metallicRoughnessTexture);
    source.occlusionTexture = toRel(gltf.occlusionTexture);
    source.emissiveTexture = toRel(gltf.emissiveTexture);
    return source;
}

} // namespace

CookResult MaterialCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    Result<MaterialSource> source = Result<MaterialSource>::Fail("uninitialized");
    if (GltfImporter::IsSubAsset(metadata.rawPath)) {
        fs::path modelRawPath;
        size_t materialIndex = 0;
        bool isMesh = false;
        if (!GltfImporter::ParseSubAsset(metadata.rawPath, modelRawPath, materialIndex, isMesh) || isMesh) {
            result.success = false;
            result.error = "material cooker got an unexpected sub-asset path: " + metadata.rawPath.generic_string();
            return result;
        }
        Result<GltfScene> scene = GltfLoader::Load(context.projectRoot / modelRawPath);
        if (!scene) {
            result.success = false;
            result.error = scene.error;
            return result;
        }
        if (materialIndex >= scene.value.materials.size()) {
            result.success = false;
            result.error = "glTF material index out of range: " + metadata.rawPath.generic_string();
            return result;
        }
        source = Result<MaterialSource>::Ok(
            SourceFromGltf(scene.value.materials[materialIndex], context.projectRoot, modelRawPath));
    } else {
        source = MaterialSerializer::LoadFile(context.projectRoot / metadata.rawPath);
    }
    if (!source) {
        result.success = false;
        result.error = source.error;
        return result;
    }

    MaterialAsset asset;
    asset.id = metadata.id;
    asset.name = source.value.name.empty() ? metadata.name : source.value.name;
    asset.baseColor = source.value.baseColor;
    asset.metallic = source.value.metallic;
    asset.roughness = source.value.roughness;
    asset.normalStrength = source.value.normalStrength;
    asset.occlusionStrength = source.value.occlusionStrength;
    asset.emissiveColor = source.value.emissiveColor;
    asset.emissiveStrength = source.value.emissiveStrength;
    asset.alphaMode = source.value.alphaMode;
    asset.alphaCutoff = source.value.alphaCutoff;
    asset.tiling = source.value.tiling;
    asset.offset = source.value.offset;

    if (context.database != nullptr) {
        const ResolvedMaterialTextures resolved = MaterialImporter::ResolveTextures(source.value, *context.database);
        asset.baseColorTexture = resolved.baseColor;
        asset.normalTexture = resolved.normal;
        asset.metallicRoughnessTexture = resolved.metallicRoughness;
        asset.occlusionTexture = resolved.occlusion;
        asset.emissiveTexture = resolved.emissive;
        result.dependencies = resolved.dependencies;
    }

    const fs::path cookedAbsolute = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(AssetType::Material) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Material));

    const Status created = FileSystem::CreateDirectories(cookedAbsolute.parent_path());
    if (!created) {
        result.success = false;
        result.error = created.error;
        return result;
    }
    const Status wrote = FileSystem::WriteTextFile(cookedAbsolute, MaterialLoader::EncodeCooked(asset));
    if (!wrote) {
        result.success = false;
        result.error = wrote.error;
        return result;
    }

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
