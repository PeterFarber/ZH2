#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

class AnimationCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Animation;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
