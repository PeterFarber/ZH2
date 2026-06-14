#pragma once
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Vertex layout matches the renderer's interleaved mesh vertex (position,
// normal, tangent, uv) so cooked meshes upload directly without conversion.
struct MeshVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    glm::vec2 uv0{0.0f};
};

struct MeshSubmesh {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    AssetID materialId;
};

struct MeshAsset {
    AssetID id;
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshSubmesh> submeshes;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

} // namespace Hockey
