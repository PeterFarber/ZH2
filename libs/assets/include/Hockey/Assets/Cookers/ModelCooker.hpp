#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks the glTF model asset itself into a .model.bin referencing its generated
// mesh and material sub-assets by AssetID.
class ModelCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Model;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
