#pragma once
#include <string>

#include "Hockey/Assets/Importer.hpp"

namespace Hockey {

// Imports .scene.yaml files. Records the asset references found in the scene as
// dependencies (meshes/materials/textures/prefabs). The ECS owns the runtime
// scene representation; hockey_assets only tracks identity and dependencies.
class SceneImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override {
        return AssetType::Scene;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    ImportResult Import(const ImportContext& context) override;
};

} // namespace Hockey
