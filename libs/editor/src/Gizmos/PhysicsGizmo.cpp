#include "Hockey/Editor/Gizmos/PhysicsGizmo.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"

namespace Hockey::PhysicsGizmo {

namespace {

// Extracts world position + rotation from an entity matrix, discarding scale by
// normalising the basis vectors. Physics shapes are not scaled by the transform
// in this engine, so the overlay matches the simulated body.
void DecomposeWorld(const glm::mat4& world, glm::vec3& outPos, glm::quat& outRot) {
    outPos = glm::vec3(world[3]);

    glm::vec3 axisX(world[0]);
    glm::vec3 axisY(world[1]);
    glm::vec3 axisZ(world[2]);
    if (glm::length(axisX) > 1e-6f) {
        axisX = glm::normalize(axisX);
    }
    if (glm::length(axisY) > 1e-6f) {
        axisY = glm::normalize(axisY);
    }
    if (glm::length(axisZ) > 1e-6f) {
        axisZ = glm::normalize(axisZ);
    }
    glm::mat3 rotation(axisX, axisY, axisZ);
    outRot = glm::normalize(glm::quat_cast(rotation));
}

glm::vec4 ColliderColor(bool isTrigger) {
    return isTrigger ? PhysicsDebugColors::kTrigger : PhysicsDebugColors::kCollider;
}

void FlushTo(DebugDraw& debug, const PhysicsDebugDrawList& list) {
    for (const PhysicsDebugLine& line : list.lines) {
        debug.DrawLine(line.a, line.b, line.color);
    }
}

} // namespace

SubmitStats Submit(DebugDraw& debug, Scene& scene, const PhysicsViewState& view, const EditorContext* context) {
    SubmitStats stats;
    if (!view.showColliders && !view.showTriggers && !view.showBodyCenters && !view.showContacts) {
        return stats;
    }

    PhysicsDebugDrawList list;

    for (Entity entity : scene.GetAllEntities()) {
        if (context != nullptr && context->IsSceneHidden(entity.GetUUID())) {
            continue;
        }
        if (!entity.HasComponent<TransformComponent>()) {
            continue;
        }

        const glm::mat4 world = scene.GetWorldTransform(entity);
        glm::vec3 worldPos;
        glm::quat worldRot;
        DecomposeWorld(world, worldPos, worldRot);

        const auto colliderCenter = [&](const glm::vec3& localOffset) { return worldPos + worldRot * localOffset; };
        const auto colliderRotation = [&](const glm::vec3& eulerDegrees) {
            return worldRot * glm::quat(glm::radians(eulerDegrees));
        };
        const auto wanted = [&](bool isTrigger) { return isTrigger ? view.showTriggers : view.showColliders; };

        if (entity.HasComponent<BoxColliderComponent>()) {
            const BoxColliderComponent& box = entity.GetComponent<BoxColliderComponent>();
            if (wanted(box.isTrigger)) {
                AppendWireBox(list, colliderCenter(box.offset), box.halfExtents, colliderRotation(box.rotation),
                              ColliderColor(box.isTrigger));
                box.isTrigger ? ++stats.triggers : ++stats.colliders;
            }
        }
        if (entity.HasComponent<SphereColliderComponent>()) {
            const SphereColliderComponent& sphere = entity.GetComponent<SphereColliderComponent>();
            if (wanted(sphere.isTrigger)) {
                AppendWireSphere(list, colliderCenter(sphere.offset), sphere.radius, ColliderColor(sphere.isTrigger));
                sphere.isTrigger ? ++stats.triggers : ++stats.colliders;
            }
        }
        if (entity.HasComponent<CapsuleColliderComponent>()) {
            const CapsuleColliderComponent& capsule = entity.GetComponent<CapsuleColliderComponent>();
            if (wanted(capsule.isTrigger)) {
                AppendWireCapsule(list, colliderCenter(capsule.offset), capsule.radius, capsule.halfHeight,
                                  colliderRotation(capsule.rotation), ColliderColor(capsule.isTrigger));
                capsule.isTrigger ? ++stats.triggers : ++stats.colliders;
            }
        }
        if (entity.HasComponent<CylinderColliderComponent>()) {
            const CylinderColliderComponent& cylinder = entity.GetComponent<CylinderColliderComponent>();
            if (wanted(cylinder.isTrigger)) {
                AppendWireCylinder(list, colliderCenter(cylinder.offset), cylinder.radius, cylinder.halfHeight,
                                   colliderRotation(cylinder.rotation), ColliderColor(cylinder.isTrigger));
                cylinder.isTrigger ? ++stats.triggers : ++stats.colliders;
            }
        }

        if (entity.HasComponent<MeshColliderComponent>()) {
            const MeshColliderComponent& meshCol = entity.GetComponent<MeshColliderComponent>();
            if (wanted(meshCol.isTrigger)) {
                // The true mesh hull is built outside hockey_physics (asset bridge,
                // deferred), so the gizmo can't draw real geometry yet. Mark the
                // entity with a unit placeholder box so authors can see the mesh
                // collider exists and where it sits.
                AppendWireBox(list, worldPos, glm::vec3(0.5f), worldRot, ColliderColor(meshCol.isTrigger));
                meshCol.isTrigger ? ++stats.triggers : ++stats.colliders;
            }
        }

        if (view.showBodyCenters && entity.HasComponent<RigidBodyComponent>()) {
            AppendCross(list, worldPos, 0.25f, PhysicsDebugColors::kBodyCenter);
            ++stats.bodyCenters;
        }
    }

    if (view.showContacts) {
        for (const glm::vec3& point : view.contactPoints) {
            AppendCross(list, point, 0.15f, PhysicsDebugColors::kContact);
            ++stats.contacts;
        }
    }

    stats.lines = static_cast<std::uint32_t>(list.lines.size());
    FlushTo(debug, list);
    return stats;
}

SubmitStats Submit(DebugDraw& debug, Scene& scene, const PhysicsViewState& view) {
    return Submit(debug, scene, view, nullptr);
}

SubmitStats Submit(DebugDraw& debug, EditorContext& context) {
    if (context.activeScene == nullptr) {
        return {};
    }
    return Submit(debug, *context.activeScene, context.physicsView, &context);
}

} // namespace Hockey::PhysicsGizmo
