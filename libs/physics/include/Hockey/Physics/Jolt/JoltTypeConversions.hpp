#pragma once

#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Hockey::JoltDetail {

JPH::Vec3 ToJolt(const glm::vec3& v);
// Real-vector position (== Vec3 unless Jolt is built in double precision).
JPH::RVec3 ToJoltR(const glm::vec3& v);
glm::vec3 ToGlm(JPH::Vec3Arg v);
// Converts a real position vector back to glm (handles double precision).
glm::vec3 RToGlm(JPH::RVec3Arg v);

JPH::Quat ToJolt(const glm::quat& q);
glm::quat ToGlm(JPH::QuatArg q);

// Converts Euler angles (degrees, glm intrinsic order) into a Jolt quaternion,
// matching the ECS TransformComponent / collider-offset convention.
JPH::Quat EulerDegreesToJolt(const glm::vec3& eulerDegrees);

} // namespace Hockey::JoltDetail
