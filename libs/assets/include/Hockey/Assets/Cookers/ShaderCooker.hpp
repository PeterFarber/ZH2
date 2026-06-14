#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks GLSL shaders. .vert/.frag/.comp are compiled to SPIR-V (.spv) via
// shaderc with #include resolution; .glsl include libraries are copied verbatim
// so they remain tracked/cooked. Reports #include dependencies.
class ShaderCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Shader;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
