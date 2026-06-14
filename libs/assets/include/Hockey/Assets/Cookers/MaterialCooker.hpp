#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks an authored .material.yaml into the cooked material format: resolves
// texture paths to AssetIDs and writes data/cooked/assets/materials/<id>.material.yaml.
// Reports the resolved texture AssetIDs as dependencies.
class MaterialCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Material;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
