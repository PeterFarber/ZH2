#include "Hockey/Physics/Jolt/JoltDebugRenderer.hpp"

#include <glm/gtc/quaternion.hpp>

#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

namespace Hockey::JoltDetail {

void CollectBodyDebugDraw(const JPH::BodyInterface& bodyInterface,
                          const std::unordered_map<JPH::BodyID, BodyDebugInfo, BodyIDHash>& bodies,
                          PhysicsDebugDrawList& outList) {
    for (const auto& [id, info] : bodies) {
        if (!bodyInterface.IsAdded(id)) {
            continue;
        }

        JPH::RVec3 jpos;
        JPH::Quat jrot;
        bodyInterface.GetPositionAndRotation(id, jpos, jrot);
        const glm::vec3 bodyPos = RToGlm(jpos);
        const glm::quat bodyRot = ToGlm(jrot);

        AppendCross(outList, bodyPos, 0.1f, PhysicsDebugColors::kBodyCenter);

        for (const DebugCollider& collider : info.colliders) {
            const glm::vec4 color = collider.isTrigger ? PhysicsDebugColors::kTrigger : PhysicsDebugColors::kCollider;
            const glm::quat localRot = glm::quat(glm::radians(collider.eulerDegrees));
            const glm::quat worldRot = bodyRot * localRot;
            const glm::vec3 worldPos = bodyPos + bodyRot * collider.offset;

            switch (collider.kind) {
            case DebugCollider::Kind::Box:
                AppendWireBox(outList, worldPos, collider.halfExtents, worldRot, color);
                break;
            case DebugCollider::Kind::Sphere:
                AppendWireSphere(outList, worldPos, collider.radius, color);
                break;
            case DebugCollider::Kind::Capsule:
                AppendWireCapsule(outList, worldPos, collider.radius, collider.halfHeight, worldRot, color);
                break;
            case DebugCollider::Kind::Cylinder:
                AppendWireCylinder(outList, worldPos, collider.radius, collider.halfHeight, worldRot, color);
                break;
            }
        }
    }
}

void JoltPhysicsWorld::CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const {
    if (!m_Initialized) {
        return;
    }
    CollectBodyDebugDraw(m_System->GetBodyInterface(), m_BodyDebug, outDrawList);
}

} // namespace Hockey::JoltDetail
