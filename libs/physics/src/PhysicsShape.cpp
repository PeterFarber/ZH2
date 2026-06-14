#include "Hockey/Physics/PhysicsShape.hpp"

// JoltShapeFactory.hpp pulls in JoltCommon.hpp (which includes <Jolt/Jolt.h>
// first); it must precede any bare <Jolt/...> header so Jolt's configuration
// macros are set up before those headers are parsed.
#include "Hockey/Physics/Jolt/JoltShapeFactory.hpp"
#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace Hockey {

bool IsValidBoxCollider(const BoxColliderComponent& c) {
    return c.halfExtents.x > 0.0f && c.halfExtents.y > 0.0f && c.halfExtents.z > 0.0f;
}

bool IsValidSphereCollider(const SphereColliderComponent& c) {
    return c.radius > 0.0f;
}

bool IsValidCapsuleCollider(const CapsuleColliderComponent& c) {
    return c.radius > 0.0f && c.halfHeight > 0.0f;
}

bool IsValidCylinderCollider(const CylinderColliderComponent& c) {
    return c.radius > 0.0f && c.halfHeight > 0.0f;
}

namespace {

ShapeBuildResult Describe(const JPH::Ref<JPH::Shape>& shape, std::string error) {
    ShapeBuildResult result;
    if (shape == nullptr) {
        result.success = false;
        result.error = std::move(error);
        return result;
    }
    result.success = true;
    const JPH::AABox bounds = shape->GetLocalBounds();
    result.boundsMin = JoltDetail::ToGlm(bounds.mMin);
    result.boundsMax = JoltDetail::ToGlm(bounds.mMax);
    result.volume = shape->GetVolume();
    return result;
}

} // namespace

ShapeBuildResult TryBuildBoxShape(const BoxColliderComponent& c) {
    std::string error;
    JPH::Ref<JPH::Shape> shape = JoltDetail::CreateBoxShape(c, error);
    return Describe(shape, error);
}

ShapeBuildResult TryBuildSphereShape(const SphereColliderComponent& c) {
    std::string error;
    JPH::Ref<JPH::Shape> shape = JoltDetail::CreateSphereShape(c, error);
    return Describe(shape, error);
}

ShapeBuildResult TryBuildCapsuleShape(const CapsuleColliderComponent& c) {
    std::string error;
    JPH::Ref<JPH::Shape> shape = JoltDetail::CreateCapsuleShape(c, error);
    return Describe(shape, error);
}

ShapeBuildResult TryBuildCylinderShape(const CylinderColliderComponent& c) {
    std::string error;
    JPH::Ref<JPH::Shape> shape = JoltDetail::CreateCylinderShape(c, error);
    return Describe(shape, error);
}

ShapeBuildResult TryBuildMeshShape(const MeshColliderComponent& c) {
    std::string error;
    JPH::Ref<JPH::Shape> shape = JoltDetail::CreateMeshShape(c, error);
    return Describe(shape, error);
}

} // namespace Hockey
