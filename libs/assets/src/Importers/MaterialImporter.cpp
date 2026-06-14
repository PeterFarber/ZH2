#include "Hockey/Assets/Importers/MaterialImporter.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"

#include <algorithm>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
AssetID ResolveOne(const std::string& path, AssetDatabase& database, std::vector<AssetID>& deps) {
    if (path.empty()) {
        return AssetID();
    }
    const fs::path normalized = AssetPath::Normalize(path);
    const AssetID id = database.GetOrCreateIDForPath(normalized, AssetType::Texture);
    if (id.IsValid() && std::find(deps.begin(), deps.end(), id) == deps.end()) {
        deps.push_back(id);
    }
    return id;
}
} // namespace

bool MaterialImporter::SupportsExtension(const std::string& extension) const {
    // Materials are matched by the compound ".material.yaml" suffix at the
    // manager level; a bare ".yaml" importer must not greedily claim scenes or
    // prefabs, so this importer is selected explicitly by asset type instead.
    (void)extension;
    return false;
}

ResolvedMaterialTextures MaterialImporter::ResolveTextures(const MaterialSource& source, AssetDatabase& database) {
    ResolvedMaterialTextures resolved;
    resolved.baseColor = ResolveOne(source.baseColorTexture, database, resolved.dependencies);
    resolved.normal = ResolveOne(source.normalTexture, database, resolved.dependencies);
    resolved.metallicRoughness = ResolveOne(source.metallicRoughnessTexture, database, resolved.dependencies);
    resolved.occlusion = ResolveOne(source.occlusionTexture, database, resolved.dependencies);
    resolved.emissive = ResolveOne(source.emissiveTexture, database, resolved.dependencies);
    return resolved;
}

ImportResult MaterialImporter::Import(const ImportContext& context) {
    ImportResult result;

    Result<MaterialSource> source = MaterialSerializer::LoadFile(context.rawPath);
    if (!source) {
        result.success = false;
        result.error = source.error;
        return result;
    }

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Material;
    result.metadata.name = source.value.name.empty() ? context.rawPath.stem().string() : source.value.name;

    if (context.database != nullptr) {
        const ResolvedMaterialTextures resolved = ResolveTextures(source.value, *context.database);
        result.metadata.dependencies = resolved.dependencies;
    }
    return result;
}

} // namespace Hockey
