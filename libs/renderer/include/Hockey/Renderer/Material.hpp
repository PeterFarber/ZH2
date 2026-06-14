#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

struct MaterialDesc {
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};

    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;

    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 0.0f;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;

    TextureHandle baseColorTexture;
    TextureHandle normalTexture;
    TextureHandle metallicRoughnessTexture;
    TextureHandle occlusionTexture;
    TextureHandle emissiveTexture;

    std::string debugName;
};

enum class BuiltInMaterial {
    DefaultWhite,
    ErrorMagenta,
    Ice,
    PuckBlack,
    HomeJersey,
    AwayJersey,
    Boards,
    Glass,
    GoalNet,
    DebugCollider,
    DebugTrigger
};

MaterialDesc MakeBuiltInMaterial(BuiltInMaterial material);
const char* BuiltInMaterialName(BuiltInMaterial material);

} // namespace Hockey
