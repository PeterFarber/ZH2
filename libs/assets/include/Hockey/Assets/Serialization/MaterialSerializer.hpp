#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Authored ("raw") material description. Texture slots reference other raw
// assets by project-relative path; the importer/cooker resolve those paths to
// AssetIDs. This is the editor-facing representation written to
// <name>.material.yaml. The cooked representation (texture paths resolved to
// AssetIDs) is owned by MaterialLoader.
struct MaterialSource {
    std::string name;

    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 0.0f;

    std::string alphaMode = "Opaque"; // Opaque | Mask | Blend
    float alphaCutoff = 0.5f;

    // Project-relative raw texture paths (empty == no texture for that slot).
    std::string baseColorTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    std::string occlusionTexture;
    std::string emissiveTexture;
};

class MaterialSerializer {
public:
    static std::string Serialize(const MaterialSource& source);
    static Result<MaterialSource> Deserialize(const std::string& text);

    static Result<MaterialSource> LoadFile(const std::filesystem::path& path);
    static Status SaveFile(const std::filesystem::path& path, const MaterialSource& source);

    // Non-empty texture paths referenced by the material, in slot order.
    static std::vector<std::string> TexturePaths(const MaterialSource& source);
};

} // namespace Hockey
