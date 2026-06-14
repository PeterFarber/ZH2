#pragma once

#include <cstdint>

#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

namespace Hockey::JoltDetail {

JPH::EMotionType ToMotionType(RigidBodyType type);

// Builds Jolt body-creation settings from a rigid body component, its resolved
// material and collision shape, and a world-space pose. entityUserData is the
// owning entity's UUID value, stored in mUserData so contact events / sync can
// recover the entity from a Jolt body.
JPH::BodyCreationSettings MakeBodyCreationSettings(const RigidBodyComponent& rb, const PhysicsMaterial& material,
                                                   const JPH::Ref<JPH::Shape>& shape, JPH::RVec3Arg position,
                                                   JPH::QuatArg rotation, bool isSensor, bool enableSleeping,
                                                   std::uint64_t entityUserData);

} // namespace Hockey::JoltDetail
