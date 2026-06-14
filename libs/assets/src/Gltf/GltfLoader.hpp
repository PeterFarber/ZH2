#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// CPU-side material description extracted from a glTF material. Texture slots
// hold the raw texture URI exactly as authored (relative to the glTF file's
// directory); resolution to AssetIDs happens in the importer/cooker.
struct GltfMaterialData {
    std::string name;
    glm::vec4 baseColor{1.0f};
    float metallic = 1.0f;
    float roughness = 1.0f;
    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 1.0f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;
    std::string alphaMode = "Opaque";
    float alphaCutoff = 0.5f;

    std::string baseColorTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    std::string occlusionTexture;
    std::string emissiveTexture;
};

// One glTF mesh flattened to a single interleaved vertex/index buffer. Each
// glTF primitive becomes a submesh; submeshMaterialIndex[i] is the glTF
// material index for submesh i (-1 when the primitive has no material).
struct GltfMeshData {
    std::string name;
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshSubmesh> submeshes;
    std::vector<int> submeshMaterialIndex;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

struct GltfScene {
    std::vector<GltfMeshData> meshes;
    std::vector<GltfMaterialData> materials;
};

// Parses a .gltf/.glb file into engine-side CPU structures using fastgltf.
class GltfLoader {
public:
    static Result<GltfScene> Load(const std::filesystem::path& gltfPath);
};

} // namespace Hockey
