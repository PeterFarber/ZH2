#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks .prefab.yaml: validates and copies to the cooked prefabs folder, and
// reports asset references as dependencies.
class PrefabCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Prefab;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
