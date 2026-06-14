#pragma once

#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <string>

#include <glm/glm.hpp>

#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "Hockey/Physics/PhysicsComponents.hpp"

namespace Hockey {
class Entity;
}

namespace Hockey::JoltDetail {

// Each Create* returns a null Ref and fills outError on failure (e.g. invalid
// sizes, or a mesh collider whose data physics cannot resolve).
JPH::Ref<JPH::Shape> CreateBoxShape(const BoxColliderComponent& c, std::string& outError);
JPH::Ref<JPH::Shape> CreateSphereShape(const SphereColliderComponent& c, std::string& outError);
JPH::Ref<JPH::Shape> CreateCapsuleShape(const CapsuleColliderComponent& c, std::string& outError);
JPH::Ref<JPH::Shape> CreateCylinderShape(const CylinderColliderComponent& c, std::string& outError);
JPH::Ref<JPH::Shape> CreateMeshShape(const MeshColliderComponent& c, std::string& outError);

// Wraps a base shape in a rotated/translated shape when the offset/rotation is
// non-identity. Returns the input shape unchanged otherwise.
JPH::Ref<JPH::Shape> ApplyOffset(const JPH::Ref<JPH::Shape>& shape, const glm::vec3& offset,
                                 const glm::vec3& eulerDegrees, std::string& outError);

// Builds the combined collision shape for an entity (a compound shape when it
// has multiple colliders). Returns null + error when no valid collider exists.
// Also reports, via outIsTrigger, whether every collider on the entity is a
// trigger (sensor) so the caller can mark the whole body as a sensor.
JPH::Ref<JPH::Shape> CreateEntityShape(const Entity& entity, std::string& outError, bool& outAllTriggers,
                                       bool& outAnyTrigger);

} // namespace Hockey::JoltDetail
