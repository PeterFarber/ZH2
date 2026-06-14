#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks .scene.yaml: validates the YAML, copies it to the cooked scenes folder,
// and reports the asset references it contains as dependencies.
class SceneCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Scene;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
