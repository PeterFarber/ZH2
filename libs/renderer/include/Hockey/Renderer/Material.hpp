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

    // UV transform applied to all texture lookups: scale (tiling) then offset.
    glm::vec2 tiling{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};

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
    DebugTrigger,
    // Opaque solid colors, handy for prototyping and distinguishing objects.
    SolidRed,
    SolidGreen,
    SolidBlue,
    SolidAmber
};

// Number of built-in materials; keep in sync with the enum above.
inline constexpr int kBuiltInMaterialCount = 15;

MaterialDesc MakeBuiltInMaterial(BuiltInMaterial material);
const char* BuiltInMaterialName(BuiltInMaterial material);

} // namespace Hockey
