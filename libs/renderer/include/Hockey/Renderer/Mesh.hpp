#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Hockey {

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    glm::vec2 uv{0.0f};
};

struct MeshDesc {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string debugName;
};

enum class BuiltInMesh { Cube, Sphere, Capsule, Cylinder, Plane, PuckCylinder, PlayerCapsule, RinkPlane };

// Procedural generators for the built-in meshes.
MeshDesc MakeCubeMesh(float size = 1.0f);
MeshDesc MakeSphereMesh(float radius = 0.5f, uint32_t segments = 24, uint32_t rings = 16);
MeshDesc MakeCylinderMesh(float radius = 0.5f, float height = 1.0f, uint32_t segments = 24);
MeshDesc MakeCapsuleMesh(float radius = 0.5f, float height = 1.0f, uint32_t segments = 16);
MeshDesc MakePlaneMesh(float width = 1.0f, float depth = 1.0f, uint32_t subdivisions = 1);
MeshDesc MakeBuiltInMesh(BuiltInMesh mesh);

const char* BuiltInMeshName(BuiltInMesh mesh);

} // namespace Hockey
