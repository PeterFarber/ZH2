#pragma once
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Importer.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"

namespace Hockey {

class AssetDatabase;

// Texture slot AssetIDs resolved from a MaterialSource against the database,
// plus the de-duplicated dependency list. Used by both the material importer
// (to record dependencies) and the material cooker (to build the cooked asset)
// so they agree on identity.
struct ResolvedMaterialTextures {
    AssetID baseColor;
    AssetID normal;
    AssetID metallicRoughness;
    AssetID occlusion;
    AssetID emissive;
    std::vector<AssetID> dependencies;
};

// Imports authored .material.yaml files: parses the source, records texture
// dependencies. Cooking is handled by MaterialCooker.
class MaterialImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override {
        return AssetType::Material;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    ImportResult Import(const ImportContext& context) override;

    // Resolves each non-empty texture path to a stable AssetID via the database.
    // Paths are interpreted relative to the project root (the raw yaml stores
    // project-relative paths). Shared by importer and cooker.
    static ResolvedMaterialTextures ResolveTextures(const MaterialSource& source, AssetDatabase& database);
};

} // namespace Hockey
