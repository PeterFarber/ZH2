#include "Hockey/Physics/Jolt/JoltBodyInterface.hpp"

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/MassProperties.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

namespace Hockey::JoltDetail {

JPH::EMotionType ToMotionType(RigidBodyType type) {
    switch (type) {
    case RigidBodyType::Static:
        return JPH::EMotionType::Static;
    case RigidBodyType::Kinematic:
        return JPH::EMotionType::Kinematic;
    case RigidBodyType::Dynamic:
        return JPH::EMotionType::Dynamic;
    }
    return JPH::EMotionType::Static;
}

namespace {

JPH::EAllowedDOFs ComputeAllowedDOFs(const RigidBodyComponent& rb) {
    JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
    if (rb.lockTranslationX) {
        dofs &= ~JPH::EAllowedDOFs::TranslationX;
    }
    if (rb.lockTranslationY) {
        dofs &= ~JPH::EAllowedDOFs::TranslationY;
    }
    if (rb.lockTranslationZ) {
        dofs &= ~JPH::EAllowedDOFs::TranslationZ;
    }
    if (rb.lockRotationX) {
        dofs &= ~JPH::EAllowedDOFs::RotationX;
    }
    if (rb.lockRotationY) {
        dofs &= ~JPH::EAllowedDOFs::RotationY;
    }
    if (rb.lockRotationZ) {
        dofs &= ~JPH::EAllowedDOFs::RotationZ;
    }
    // Jolt requires at least one allowed DOF; fall back to all if fully locked
    // (a fully locked dynamic body is better expressed as Kinematic/Static).
    if (dofs == JPH::EAllowedDOFs::None) {
        dofs = JPH::EAllowedDOFs::All;
    }
    return dofs;
}

} // namespace

JPH::BodyCreationSettings MakeBodyCreationSettings(const RigidBodyComponent& rb, const PhysicsMaterial& material,
                                                   const JPH::Ref<JPH::Shape>& shape, JPH::RVec3Arg position,
                                                   JPH::QuatArg rotation, bool isSensor, bool enableSleeping,
                                                   std::uint64_t entityUserData) {
    const JPH::EMotionType motionType = ToMotionType(rb.type);
    const auto objectLayer = static_cast<JPH::ObjectLayer>(static_cast<int>(rb.layer));

    JPH::BodyCreationSettings settings(shape, position, rotation, motionType, objectLayer);

    settings.mFriction = material.friction;
    settings.mRestitution = material.restitution;
    settings.mLinearDamping = JPH::max(0.0f, rb.linearDamping + material.linearDamping);
    settings.mAngularDamping = JPH::max(0.0f, rb.angularDamping + material.angularDamping);
    settings.mGravityFactor = rb.useGravity ? 1.0f : 0.0f;
    settings.mAllowSleeping = rb.allowSleeping && enableSleeping;
    settings.mIsSensor = isSensor;
    settings.mUserData = entityUserData;

    if (motionType == JPH::EMotionType::Dynamic) {
        settings.mAllowedDOFs = ComputeAllowedDOFs(rb);
        if (rb.collisionDetection == CollisionDetectionMode::Continuous) {
            settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
        }
        if (rb.mass > 0.0f) {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = rb.mass;
        }
        settings.mLinearVelocity = ToJolt(rb.initialLinearVelocity);
        settings.mAngularVelocity = ToJolt(rb.initialAngularVelocity);
    }

    return settings;
}

} // namespace Hockey::JoltDetail
