#include "Hockey/Physics/PhysicsValidation.hpp"

#include <cstdint>
#include <string>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsLayer.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

namespace Hockey {

namespace {

using Severity = SceneValidationIssue::Severity;

void AddIssue(std::vector<SceneValidationIssue>& issues, Severity severity, std::string message, UUID entityId) {
    issues.push_back({severity, std::move(message), entityId});
}

bool HasAnyColliderHandle(const entt::registry& registry, entt::entity handle) {
    return registry.all_of<BoxColliderComponent>(handle) || registry.all_of<SphereColliderComponent>(handle) ||
           registry.all_of<CapsuleColliderComponent>(handle) || registry.all_of<CylinderColliderComponent>(handle) ||
           registry.all_of<MeshColliderComponent>(handle);
}

bool AnyColliderIsTrigger(const entt::registry& registry, entt::entity handle) {
    if (const auto* box = registry.try_get<BoxColliderComponent>(handle); box != nullptr && box->isTrigger) {
        return true;
    }
    if (const auto* sphere = registry.try_get<SphereColliderComponent>(handle);
        sphere != nullptr && sphere->isTrigger) {
        return true;
    }
    if (const auto* capsule = registry.try_get<CapsuleColliderComponent>(handle);
        capsule != nullptr && capsule->isTrigger) {
        return true;
    }
    if (const auto* cylinder = registry.try_get<CylinderColliderComponent>(handle);
        cylinder != nullptr && cylinder->isTrigger) {
        return true;
    }
    if (const auto* mesh = registry.try_get<MeshColliderComponent>(handle); mesh != nullptr && mesh->isTrigger) {
        return true;
    }
    return false;
}

void ValidateColliderSizes(const entt::registry& registry, entt::entity handle, UUID id,
                           std::vector<SceneValidationIssue>& issues) {
    if (const auto* box = registry.try_get<BoxColliderComponent>(handle)) {
        if (box->halfExtents.x <= 0.0f || box->halfExtents.y <= 0.0f || box->halfExtents.z <= 0.0f) {
            AddIssue(issues, Severity::Error, "BoxCollider has a zero/negative half-extent", id);
        }
    }
    if (const auto* sphere = registry.try_get<SphereColliderComponent>(handle)) {
        if (sphere->radius <= 0.0f) {
            AddIssue(issues, Severity::Error, "SphereCollider has a zero/negative radius", id);
        }
    }
    if (const auto* capsule = registry.try_get<CapsuleColliderComponent>(handle)) {
        if (capsule->radius <= 0.0f || capsule->halfHeight <= 0.0f) {
            AddIssue(issues, Severity::Error, "CapsuleCollider has a zero/negative radius or half-height", id);
        }
    }
    if (const auto* cylinder = registry.try_get<CylinderColliderComponent>(handle)) {
        if (cylinder->radius <= 0.0f || cylinder->halfHeight <= 0.0f) {
            AddIssue(issues, Severity::Error, "CylinderCollider has a zero/negative radius or half-height", id);
        }
    }
}

} // namespace

void ValidatePhysicsScene(const Scene& scene, std::vector<SceneValidationIssue>& issues) {
    const entt::registry& registry = scene.Registry();
    const PhysicsMaterialRegistry& materials = PhysicsMaterialRegistry::Get();
    const bool haveMaterials = !materials.All().empty();

    auto idView = registry.view<const IDComponent>();
    for (const entt::entity handle : idView) {
        const UUID id = idView.get<const IDComponent>(handle).id;

        const auto* rigidBody = registry.try_get<RigidBodyComponent>(handle);
        const bool hasCollider = HasAnyColliderHandle(registry, handle);

        if (rigidBody != nullptr) {
            if (!hasCollider) {
                AddIssue(issues, Severity::Warning, "RigidBody has no collider; it will not create a physics body", id);
            }
            if (rigidBody->mass < 0.0f) {
                AddIssue(issues, Severity::Error, "RigidBody mass is negative", id);
            }
            if (rigidBody->type == RigidBodyType::Dynamic && rigidBody->mass <= 0.0f) {
                AddIssue(issues, Severity::Error, "Dynamic RigidBody must have positive mass", id);
            }
            if (!IsValidPhysicsLayer(static_cast<std::uint16_t>(rigidBody->layer))) {
                AddIssue(issues, Severity::Error, "RigidBody references an invalid physics layer", id);
            }
            if (haveMaterials && !rigidBody->materialName.empty() && !materials.Contains(rigidBody->materialName)) {
                AddIssue(issues, Severity::Warning,
                         "RigidBody material '" + rigidBody->materialName + "' is not in the material registry", id);
            }
        } else if (hasCollider) {
            AddIssue(issues, Severity::Warning, "Collider has no RigidBodyComponent; it will not create a physics body",
                     id);
        }

        ValidateColliderSizes(registry, handle, id, issues);

        if (const auto* mesh = registry.try_get<MeshColliderComponent>(handle)) {
            if (rigidBody != nullptr && rigidBody->type == RigidBodyType::Dynamic && !mesh->convex) {
                AddIssue(issues, Severity::Error,
                         "Dynamic body uses a non-convex MeshCollider (only convex meshes can be dynamic)", id);
            }
        }

        // A trigger-shaped collider should be paired with a TriggerComponent so
        // gameplay can react to enter/exit.
        if (AnyColliderIsTrigger(registry, handle) && !registry.all_of<TriggerComponent>(handle)) {
            AddIssue(issues, Severity::Warning, "Trigger collider has no TriggerComponent", id);
        }

        // Hockey setup hints (this phase only builds physics, not gameplay).
        if (registry.all_of<GoalComponent>(handle)) {
            const bool triggerReady =
                AnyColliderIsTrigger(registry, handle) && registry.all_of<TriggerComponent>(handle);
            if (!triggerReady) {
                AddIssue(issues, Severity::Warning,
                         "Goal entity is missing a trigger collider + TriggerComponent for goal detection", id);
            }
        }

        if (registry.all_of<PuckComponent>(handle)) {
            if (rigidBody == nullptr || !hasCollider) {
                AddIssue(issues, Severity::Warning, "Puck entity is missing a RigidBody and/or collider", id);
            } else if (haveMaterials && !materials.Contains(rigidBody->materialName)) {
                AddIssue(issues, Severity::Warning, "Puck entity has no valid physics material", id);
            }
        }

        // Players and goalies should be represented by a capsule. Only flag
        // entities already set up as physics actors so pure spawn markers stay
        // quiet (this phase builds physics, not the gameplay spawn system).
        if (const auto* role = registry.try_get<PlayerRoleComponent>(handle)) {
            const char* roleName = role->role == PlayerRole::Goalie ? "Goalie" : "Skater";
            const auto* capsule = registry.try_get<CapsuleColliderComponent>(handle);
            if (capsule != nullptr) {
                if (capsule->radius <= 0.0f || capsule->halfHeight <= 0.0f) {
                    AddIssue(issues, Severity::Warning,
                             std::string(roleName) + " capsule has invalid (zero/negative) dimensions", id);
                }
            } else if (rigidBody != nullptr || hasCollider) {
                AddIssue(issues, Severity::Warning,
                         std::string(roleName) + " physics body should use a CapsuleCollider", id);
            }
        }
    }
}

void RegisterPhysicsValidation() {
    static bool s_Registered = false;
    if (s_Registered) {
        return;
    }
    SceneValidator::RegisterExternal(ValidatePhysicsScene);
    s_Registered = true;
}

} // namespace Hockey
