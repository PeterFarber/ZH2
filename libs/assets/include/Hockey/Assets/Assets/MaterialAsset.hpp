#pragma once
#include <string>

#include <glm/glm.hpp>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// PBR metallic-roughness material. Texture slots reference TextureAssets by id;
// an invalid id means "no texture" and the renderer substitutes a default.
struct MaterialAsset {
    AssetID id;
    std::string name;

    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 0.0f;

    AssetID baseColorTexture;
    AssetID normalTexture;
    AssetID metallicRoughnessTexture;
    AssetID occlusionTexture;
    AssetID emissiveTexture;

    std::string alphaMode = "Opaque"; // Opaque | Mask | Blend
    float alphaCutoff = 0.5f;
    glm::vec2 tiling{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};
};

} // namespace Hockey
