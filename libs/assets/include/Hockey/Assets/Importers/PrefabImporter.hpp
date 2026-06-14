#pragma once
#include <string>

#include "Hockey/Assets/Importer.hpp"

namespace Hockey {

// Imports .prefab.yaml files, tracking asset references as dependencies.
class PrefabImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override {
        return AssetType::Prefab;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    ImportResult Import(const ImportContext& context) override;
};

} // namespace Hockey
