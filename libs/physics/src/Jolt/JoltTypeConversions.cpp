#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

namespace Hockey::JoltDetail {

JPH::Vec3 ToJolt(const glm::vec3& v) {
    return JPH::Vec3(v.x, v.y, v.z);
}

JPH::RVec3 ToJoltR(const glm::vec3& v) {
    return JPH::RVec3(v.x, v.y, v.z);
}

glm::vec3 ToGlm(JPH::Vec3Arg v) {
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

glm::vec3 RToGlm(JPH::RVec3Arg v) {
    return glm::vec3(static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
}

JPH::Quat ToJolt(const glm::quat& q) {
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

glm::quat ToGlm(JPH::QuatArg q) {
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

JPH::Quat EulerDegreesToJolt(const glm::vec3& eulerDegrees) {
    return ToJolt(glm::quat(glm::radians(eulerDegrees)));
}

} // namespace Hockey::JoltDetail
