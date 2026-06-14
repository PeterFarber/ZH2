#include "Hockey/Renderer/Material.hpp"

namespace Hockey {

MaterialDesc MakeBuiltInMaterial(BuiltInMaterial material) {
    MaterialDesc desc;
    switch (material) {
    case BuiltInMaterial::DefaultWhite:
        desc.baseColor = {0.8f, 0.8f, 0.8f, 1.0f};
        desc.roughness = 0.6f;
        desc.debugName = "DefaultWhite";
        break;
    case BuiltInMaterial::ErrorMagenta:
        desc.baseColor = {1.0f, 0.0f, 1.0f, 1.0f};
        desc.roughness = 0.9f;
        desc.emissiveColor = {0.4f, 0.0f, 0.4f};
        desc.emissiveStrength = 1.0f;
        desc.debugName = "ErrorMagenta";
        break;
    case BuiltInMaterial::Ice:
        desc.baseColor = {0.92f, 0.95f, 1.0f, 1.0f};
        desc.metallic = 0.0f;
        desc.roughness = 0.15f;
        desc.debugName = "Ice";
        break;
    case BuiltInMaterial::PuckBlack:
        desc.baseColor = {0.03f, 0.03f, 0.03f, 1.0f};
        desc.roughness = 0.7f;
        desc.debugName = "PuckBlack";
        break;
    case BuiltInMaterial::HomeJersey:
        desc.baseColor = {0.7f, 0.1f, 0.12f, 1.0f};
        desc.roughness = 0.8f;
        desc.debugName = "HomeJersey";
        break;
    case BuiltInMaterial::AwayJersey:
        desc.baseColor = {0.95f, 0.95f, 0.97f, 1.0f};
        desc.roughness = 0.8f;
        desc.debugName = "AwayJersey";
        break;
    case BuiltInMaterial::Boards:
        desc.baseColor = {0.95f, 0.95f, 0.95f, 1.0f};
        desc.roughness = 0.5f;
        desc.debugName = "Boards";
        break;
    case BuiltInMaterial::Glass:
        desc.baseColor = {0.6f, 0.75f, 0.85f, 0.25f};
        desc.metallic = 0.0f;
        desc.roughness = 0.05f;
        desc.alphaMode = AlphaMode::Blend;
        desc.debugName = "Glass";
        break;
    case BuiltInMaterial::GoalNet:
        desc.baseColor = {0.9f, 0.9f, 0.9f, 0.5f};
        desc.roughness = 0.9f;
        desc.alphaMode = AlphaMode::Mask;
        desc.alphaCutoff = 0.5f;
        desc.debugName = "GoalNet";
        break;
    case BuiltInMaterial::DebugCollider:
        desc.baseColor = {0.1f, 0.9f, 0.2f, 0.4f};
        desc.roughness = 1.0f;
        desc.alphaMode = AlphaMode::Blend;
        desc.emissiveColor = {0.05f, 0.4f, 0.1f};
        desc.emissiveStrength = 0.5f;
        desc.debugName = "DebugCollider";
        break;
    case BuiltInMaterial::DebugTrigger:
        desc.baseColor = {0.9f, 0.7f, 0.1f, 0.35f};
        desc.roughness = 1.0f;
        desc.alphaMode = AlphaMode::Blend;
        desc.emissiveColor = {0.4f, 0.3f, 0.05f};
        desc.emissiveStrength = 0.5f;
        desc.debugName = "DebugTrigger";
        break;
    }
    return desc;
}

const char* BuiltInMaterialName(BuiltInMaterial material) {
    switch (material) {
    case BuiltInMaterial::DefaultWhite:
        return "DefaultWhite";
    case BuiltInMaterial::ErrorMagenta:
        return "ErrorMagenta";
    case BuiltInMaterial::Ice:
        return "Ice";
    case BuiltInMaterial::PuckBlack:
        return "PuckBlack";
    case BuiltInMaterial::HomeJersey:
        return "HomeJersey";
    case BuiltInMaterial::AwayJersey:
        return "AwayJersey";
    case BuiltInMaterial::Boards:
        return "Boards";
    case BuiltInMaterial::Glass:
        return "Glass";
    case BuiltInMaterial::GoalNet:
        return "GoalNet";
    case BuiltInMaterial::DebugCollider:
        return "DebugCollider";
    case BuiltInMaterial::DebugTrigger:
        return "DebugTrigger";
    }
    return "Unknown";
}

} // namespace Hockey
