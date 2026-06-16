#include "Hockey/Renderer/Mesh.hpp"

#include <cmath>

#include <glm/gtc/constants.hpp>

namespace Hockey {

namespace {

void AddQuad(MeshDesc& mesh, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v, const glm::vec3& n,
             float half) {
    const auto base = static_cast<uint32_t>(mesh.vertices.size());
    const glm::vec4 tangent(u, 1.0f);
    mesh.vertices.push_back({center + (-u - v) * half, n, tangent, {0.0f, 1.0f}});
    mesh.vertices.push_back({center + (u - v) * half, n, tangent, {1.0f, 1.0f}});
    mesh.vertices.push_back({center + (u + v) * half, n, tangent, {1.0f, 0.0f}});
    mesh.vertices.push_back({center + (-u + v) * half, n, tangent, {0.0f, 0.0f}});
    mesh.indices.insert(mesh.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

} // namespace

MeshDesc MakeCubeMesh(float size) {
    MeshDesc mesh;
    mesh.debugName = "CubeMesh";
    const float half = size * 0.5f;
    const glm::vec3 faces[6][3] = {
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  // +X
        {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},  // -X
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  // +Y
        {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},  // -Y
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   // +Z
        {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}}, // -Z
    };
    for (const auto& f : faces) {
        const glm::vec3 n = f[0];
        const glm::vec3 u = f[1];
        const glm::vec3 v = f[2];
        AddQuad(mesh, n * half, u, v, n, half);
    }
    return mesh;
}

MeshDesc MakeSphereMesh(float radius, uint32_t segments, uint32_t rings) {
    MeshDesc mesh;
    mesh.debugName = "SphereMesh";
    segments = segments < 3 ? 3 : segments;
    rings = rings < 2 ? 2 : rings;

    for (uint32_t y = 0; y <= rings; ++y) {
        const float theta = glm::pi<float>() * static_cast<float>(y) / static_cast<float>(rings);
        const float sinT = std::sin(theta);
        const float cosT = std::cos(theta);
        for (uint32_t x = 0; x <= segments; ++x) {
            const float phi = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segments);
            const float sinP = std::sin(phi);
            const float cosP = std::cos(phi);
            const glm::vec3 normal(sinT * cosP, cosT, sinT * sinP);
            const glm::vec3 tangent(-sinP, 0.0f, cosP);
            Vertex vert;
            vert.position = normal * radius;
            vert.normal = normal;
            vert.tangent = glm::vec4(tangent, 1.0f);
            vert.uv = {static_cast<float>(x) / static_cast<float>(segments),
                       static_cast<float>(y) / static_cast<float>(rings)};
            mesh.vertices.push_back(vert);
        }
    }

    const uint32_t stride = segments + 1;
    for (uint32_t y = 0; y < rings; ++y) {
        for (uint32_t x = 0; x < segments; ++x) {
            const uint32_t i0 = y * stride + x;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + stride;
            const uint32_t i3 = i2 + 1;
            mesh.indices.insert(mesh.indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }
    return mesh;
}

MeshDesc MakeCylinderMesh(float radius, float height, uint32_t segments) {
    MeshDesc mesh;
    mesh.debugName = "CylinderMesh";
    segments = segments < 3 ? 3 : segments;
    const float halfH = height * 0.5f;

    // Side wall.
    for (uint32_t x = 0; x <= segments; ++x) {
        const float phi = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segments);
        const float cosP = std::cos(phi);
        const float sinP = std::sin(phi);
        const glm::vec3 normal(cosP, 0.0f, sinP);
        const glm::vec4 tangent(-sinP, 0.0f, cosP, 1.0f);
        const float uCoord = static_cast<float>(x) / static_cast<float>(segments);
        mesh.vertices.push_back({{cosP * radius, halfH, sinP * radius}, normal, tangent, {uCoord, 0.0f}});
        mesh.vertices.push_back({{cosP * radius, -halfH, sinP * radius}, normal, tangent, {uCoord, 1.0f}});
    }
    for (uint32_t x = 0; x < segments; ++x) {
        const uint32_t i0 = x * 2;
        const uint32_t i1 = i0 + 1;
        const uint32_t i2 = i0 + 2;
        const uint32_t i3 = i0 + 3;
        mesh.indices.insert(mesh.indices.end(), {i0, i1, i2, i1, i3, i2});
    }

    // Caps.
    const auto addCap = [&](float y, const glm::vec3& normal, bool flip) {
        const auto centerIdx = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({{0.0f, y, 0.0f}, normal, {1, 0, 0, 1}, {0.5f, 0.5f}});
        const auto rimStart = static_cast<uint32_t>(mesh.vertices.size());
        for (uint32_t x = 0; x <= segments; ++x) {
            const float phi = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segments);
            const float cosP = std::cos(phi);
            const float sinP = std::sin(phi);
            mesh.vertices.push_back(
                {{cosP * radius, y, sinP * radius}, normal, {1, 0, 0, 1}, {cosP * 0.5f + 0.5f, sinP * 0.5f + 0.5f}});
        }
        for (uint32_t x = 0; x < segments; ++x) {
            const uint32_t a = rimStart + x;
            const uint32_t b = rimStart + x + 1;
            if (flip) {
                mesh.indices.insert(mesh.indices.end(), {centerIdx, b, a});
            } else {
                mesh.indices.insert(mesh.indices.end(), {centerIdx, a, b});
            }
        }
    };
    addCap(halfH, {0, 1, 0}, false);
    addCap(-halfH, {0, -1, 0}, true);
    return mesh;
}

MeshDesc MakeCapsuleMesh(float radius, float height, uint32_t segments) {
    MeshDesc mesh;
    mesh.debugName = "CapsuleMesh";
    segments = segments < 3 ? 3 : segments;
    const uint32_t rings = segments;
    const float halfH = height * 0.5f;
    const uint32_t stride = segments + 1;

    // Generates a hemisphere; topHalf=true builds the upper cap, offset along Y.
    const auto addHemisphere = [&](bool topHalf, float yOffset) {
        const auto base = static_cast<uint32_t>(mesh.vertices.size());
        const uint32_t halfRings = rings / 2;
        for (uint32_t y = 0; y <= halfRings; ++y) {
            float t = static_cast<float>(y) / static_cast<float>(halfRings); // 0..1
            float theta = topHalf ? (glm::half_pi<float>() * (1.0f - t))     // 90deg -> 0
                                  : (glm::half_pi<float>() + glm::half_pi<float>() * t);
            const float sinT = std::sin(theta);
            const float cosT = std::cos(theta);
            for (uint32_t x = 0; x <= segments; ++x) {
                const float phi = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segments);
                const glm::vec3 normal(sinT * std::cos(phi), cosT, sinT * std::sin(phi));
                Vertex vert;
                vert.position = normal * radius + glm::vec3(0.0f, yOffset, 0.0f);
                vert.normal = normal;
                vert.tangent = glm::vec4(-std::sin(phi), 0.0f, std::cos(phi), 1.0f);
                vert.uv = {static_cast<float>(x) / static_cast<float>(segments), t};
                mesh.vertices.push_back(vert);
            }
        }
        for (uint32_t y = 0; y < halfRings; ++y) {
            for (uint32_t x = 0; x < segments; ++x) {
                const uint32_t i0 = base + y * stride + x;
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = i0 + stride;
                const uint32_t i3 = i2 + 1;
                if (topHalf) {
                    mesh.indices.insert(mesh.indices.end(), {i0, i2, i1, i1, i2, i3});
                } else {
                    mesh.indices.insert(mesh.indices.end(), {i0, i1, i2, i1, i3, i2});
                }
            }
        }
    };

    // Cylinder body between the hemisphere centers.
    for (uint32_t x = 0; x <= segments; ++x) {
        const float phi = glm::two_pi<float>() * static_cast<float>(x) / static_cast<float>(segments);
        const float cosP = std::cos(phi);
        const float sinP = std::sin(phi);
        const glm::vec3 normal(cosP, 0.0f, sinP);
        const glm::vec4 tangent(-sinP, 0.0f, cosP, 1.0f);
        const float uCoord = static_cast<float>(x) / static_cast<float>(segments);
        mesh.vertices.push_back({{cosP * radius, halfH, sinP * radius}, normal, tangent, {uCoord, 0.0f}});
        mesh.vertices.push_back({{cosP * radius, -halfH, sinP * radius}, normal, tangent, {uCoord, 1.0f}});
    }
    for (uint32_t x = 0; x < segments; ++x) {
        const uint32_t i0 = x * 2;
        const uint32_t i1 = i0 + 1;
        const uint32_t i2 = i0 + 2;
        const uint32_t i3 = i0 + 3;
        mesh.indices.insert(mesh.indices.end(), {i0, i1, i2, i1, i3, i2});
    }

    addHemisphere(true, halfH);
    addHemisphere(false, -halfH);
    return mesh;
}

MeshDesc MakePlaneMesh(float width, float depth, uint32_t subdivisions) {
    MeshDesc mesh;
    mesh.debugName = "PlaneMesh";
    subdivisions = subdivisions < 1 ? 1 : subdivisions;
    const float halfW = width * 0.5f;
    const float halfD = depth * 0.5f;
    const uint32_t stride = subdivisions + 1;

    for (uint32_t z = 0; z <= subdivisions; ++z) {
        const float tz = static_cast<float>(z) / static_cast<float>(subdivisions);
        for (uint32_t x = 0; x <= subdivisions; ++x) {
            const float tx = static_cast<float>(x) / static_cast<float>(subdivisions);
            Vertex vert;
            vert.position = {(tx * 2.0f - 1.0f) * halfW, 0.0f, (tz * 2.0f - 1.0f) * halfD};
            vert.normal = {0.0f, 1.0f, 0.0f};
            vert.tangent = {1.0f, 0.0f, 0.0f, 1.0f};
            vert.uv = {tx, tz};
            mesh.vertices.push_back(vert);
        }
    }
    for (uint32_t z = 0; z < subdivisions; ++z) {
        for (uint32_t x = 0; x < subdivisions; ++x) {
            const uint32_t i0 = z * stride + x;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + stride;
            const uint32_t i3 = i2 + 1;
            mesh.indices.insert(mesh.indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }
    return mesh;
}

MeshDesc MakeBuiltInMesh(BuiltInMesh mesh) {
    switch (mesh) {
    case BuiltInMesh::Cube:
        return MakeCubeMesh(1.0f);
    case BuiltInMesh::Sphere:
        return MakeSphereMesh(0.5f, 32, 16);
    case BuiltInMesh::Capsule:
        return MakeCapsuleMesh(0.5f, 1.0f, 16);
    case BuiltInMesh::Cylinder:
        return MakeCylinderMesh(0.5f, 1.0f, 32);
    case BuiltInMesh::Plane:
        return MakePlaneMesh(1.0f, 1.0f, 1);
    case BuiltInMesh::PuckCylinder:
        return MakeCylinderMesh(0.0762f, 0.0254f, 32); // regulation hockey puck
    case BuiltInMesh::PlayerCapsule:
        return MakeCapsuleMesh(0.35f, 1.1f, 16); // ~1.8m total skater height
    case BuiltInMesh::RinkPlane:
        return MakePlaneMesh(60.96f, 25.91f, 4); // NHL rink (200ft x 85ft)
    }
    return MakeCubeMesh(1.0f);
}

const char* BuiltInMeshName(BuiltInMesh mesh) {
    switch (mesh) {
    case BuiltInMesh::Cube:
        return "Cube";
    case BuiltInMesh::Sphere:
        return "Sphere";
    case BuiltInMesh::Capsule:
        return "Capsule";
    case BuiltInMesh::Cylinder:
        return "Cylinder";
    case BuiltInMesh::Plane:
        return "Plane";
    case BuiltInMesh::PuckCylinder:
        return "PuckCylinder";
    case BuiltInMesh::PlayerCapsule:
        return "PlayerCapsule";
    case BuiltInMesh::RinkPlane:
        return "RinkPlane";
    }
    return "Unknown";
}

} // namespace Hockey
