#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Hockey/Physics/PhysicsComponents.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Collider-size validation. These are pure (no Jolt) and used by both the
// scene validator and the body builder.
// ---------------------------------------------------------------------------
bool IsValidBoxCollider(const BoxColliderComponent& c);
bool IsValidSphereCollider(const SphereColliderComponent& c);
bool IsValidCapsuleCollider(const CapsuleColliderComponent& c);
bool IsValidCylinderCollider(const CylinderColliderComponent& c);

// ---------------------------------------------------------------------------
// Result of attempting to build a Jolt collision shape from a collider. Lets
// non-physics callers (tests, tools) verify shape creation without ever seeing
// a Jolt type.
// ---------------------------------------------------------------------------
struct ShapeBuildResult {
    bool success = false;
    std::string error;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    float volume = 0.0f;
};

ShapeBuildResult TryBuildBoxShape(const BoxColliderComponent& c);
ShapeBuildResult TryBuildSphereShape(const SphereColliderComponent& c);
ShapeBuildResult TryBuildCapsuleShape(const CapsuleColliderComponent& c);
ShapeBuildResult TryBuildCylinderShape(const CylinderColliderComponent& c);
ShapeBuildResult TryBuildMeshShape(const MeshColliderComponent& c);

} // namespace Hockey
