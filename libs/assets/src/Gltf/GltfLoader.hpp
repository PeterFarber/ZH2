#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// An image stored inside the glTF/GLB itself (a buffer-view or data-URI source)
// rather than referenced as an external file. The loader rewrites the owning
// material slot to point at relativePath; the importer is responsible for
// writing these bytes to disk (relative to the model file's directory) so the
// standard texture pipeline can discover and cook them.
struct GltfEmbeddedTexture {
    std::string relativePath;       // relative to the glTF file's directory
    std::vector<std::byte> bytes;   // encoded image bytes (png/jpg/...) as authored
};

// CPU-side material description extracted from a glTF material. Texture slots
// hold the raw texture URI exactly as authored (relative to the glTF file's
// directory); resolution to AssetIDs happens in the importer/cooker. Embedded
// images are externalized to deterministic sibling paths (see embeddedTextures).
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
    // Images embedded in the file that the importer should write to disk so the
    // texture pipeline can cook them. Paths are referenced by material slots.
    std::vector<GltfEmbeddedTexture> embeddedTextures;
};

// Parses a .gltf/.glb file into engine-side CPU structures using fastgltf.
class GltfLoader {
public:
    static Result<GltfScene> Load(const std::filesystem::path& gltfPath);
};

} // namespace Hockey
