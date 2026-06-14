#include "Hockey/ECS/PrefabOverride.hpp"

#include <exception>
#include <string>

#include <entt/entt.hpp>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/YAML.hpp"

namespace Hockey {

void PrefabOverrideSet::AddOverride(const PrefabOverride& override) {
    m_Overrides.push_back(override);
}

const std::vector<PrefabOverride>& PrefabOverrideSet::Overrides() const {
    return m_Overrides;
}

namespace {

Status Fail(const std::string& message) {
    HK_CORE_ERROR("Prefab override failed: {}", message);
    return Status::Fail(message);
}

Status ApplyOne(Scene& scene, const PrefabOverride& ov) {
    Entity entity = scene.FindEntityByUUID(ov.entityId);
    if (!entity.IsValid()) {
        return Fail("entity " + ov.entityId.ToString() + " not found");
    }

    entt::registry& registry = scene.Registry();
    const entt::entity handle = entity.Handle();

    if (ov.componentName == "TransformComponent") {
        auto* transform = registry.try_get<TransformComponent>(handle);
        if (transform == nullptr) {
            return Fail("missing TransformComponent");
        }
        if (ov.fieldName == "Position") {
            if (!ReadVec3(ov.value, transform->localPosition)) {
                return Fail("invalid Position value");
            }
        } else if (ov.fieldName == "Rotation") {
            if (!ReadVec3(ov.value, transform->localRotation)) {
                return Fail("invalid Rotation value");
            }
        } else if (ov.fieldName == "Scale") {
            if (!ReadVec3(ov.value, transform->localScale)) {
                return Fail("invalid Scale value");
            }
        } else {
            return Fail("unknown TransformComponent field '" + ov.fieldName + "'");
        }
        return Status::Ok();
    }

    if (ov.componentName == "NameComponent") {
        if (ov.fieldName != "Name") {
            return Fail("unknown NameComponent field '" + ov.fieldName + "'");
        }
        registry.get<NameComponent>(handle).name = ov.value.as<std::string>();
        return Status::Ok();
    }

    if (ov.componentName == "ActiveComponent") {
        if (ov.fieldName != "Active") {
            return Fail("unknown ActiveComponent field '" + ov.fieldName + "'");
        }
        registry.get<ActiveComponent>(handle).active = ov.value.as<bool>();
        scene.RecalculateActiveInHierarchy(entity);
        return Status::Ok();
    }

    if (ov.componentName == "TeamComponent") {
        auto* component = registry.try_get<TeamComponent>(handle);
        if (component == nullptr) {
            return Fail("missing TeamComponent");
        }
        if (ov.fieldName != "Team") {
            return Fail("unknown TeamComponent field '" + ov.fieldName + "'");
        }
        component->team = TeamFromString(ov.value.as<std::string>());
        return Status::Ok();
    }

    if (ov.componentName == "PlayerRoleComponent") {
        auto* component = registry.try_get<PlayerRoleComponent>(handle);
        if (component == nullptr) {
            return Fail("missing PlayerRoleComponent");
        }
        if (ov.fieldName != "Role") {
            return Fail("unknown PlayerRoleComponent field '" + ov.fieldName + "'");
        }
        component->role = PlayerRoleFromString(ov.value.as<std::string>());
        return Status::Ok();
    }

    if (ov.componentName == "GoalComponent") {
        auto* component = registry.try_get<GoalComponent>(handle);
        if (component == nullptr) {
            return Fail("missing GoalComponent");
        }
        if (ov.fieldName != "DefendingTeam") {
            return Fail("unknown GoalComponent field '" + ov.fieldName + "'");
        }
        component->defendingTeam = TeamFromString(ov.value.as<std::string>());
        return Status::Ok();
    }

    if (ov.componentName == "PuckComponent") {
        auto* component = registry.try_get<PuckComponent>(handle);
        if (component == nullptr) {
            return Fail("missing PuckComponent");
        }
        if (ov.fieldName != "StartsInPlay") {
            return Fail("unknown PuckComponent field '" + ov.fieldName + "'");
        }
        component->startsInPlay = ov.value.as<bool>();
        return Status::Ok();
    }

    if (ov.componentName == "SpawnPointComponent") {
        auto* component = registry.try_get<SpawnPointComponent>(handle);
        if (component == nullptr) {
            return Fail("missing SpawnPointComponent");
        }
        if (ov.fieldName == "Team") {
            component->team = TeamFromString(ov.value.as<std::string>());
        } else if (ov.fieldName == "Role") {
            component->role = PlayerRoleFromString(ov.value.as<std::string>());
        } else if (ov.fieldName == "Index") {
            component->index = ov.value.as<int>();
        } else {
            return Fail("unknown SpawnPointComponent field '" + ov.fieldName + "'");
        }
        return Status::Ok();
    }

    if (ov.componentName == "FaceoffSpotComponent") {
        auto* component = registry.try_get<FaceoffSpotComponent>(handle);
        if (component == nullptr) {
            return Fail("missing FaceoffSpotComponent");
        }
        if (ov.fieldName != "Index") {
            return Fail("unknown FaceoffSpotComponent field '" + ov.fieldName + "'");
        }
        component->index = ov.value.as<int>();
        return Status::Ok();
    }

    if (ov.componentName == "RinkComponent") {
        auto* component = registry.try_get<RinkComponent>(handle);
        if (component == nullptr) {
            return Fail("missing RinkComponent");
        }
        if (ov.fieldName != "RinkName") {
            return Fail("unknown RinkComponent field '" + ov.fieldName + "'");
        }
        component->rinkName = ov.value.as<std::string>();
        return Status::Ok();
    }

    if (ov.componentName == "PlayAreaComponent") {
        auto* component = registry.try_get<PlayAreaComponent>(handle);
        if (component == nullptr) {
            return Fail("missing PlayAreaComponent");
        }
        if (ov.fieldName != "HalfExtents") {
            return Fail("unknown PlayAreaComponent field '" + ov.fieldName + "'");
        }
        if (!ReadVec3(ov.value, component->halfExtents)) {
            return Fail("invalid HalfExtents value");
        }
        return Status::Ok();
    }

    if (ov.componentName == "CameraRigMarkerComponent") {
        auto* component = registry.try_get<CameraRigMarkerComponent>(handle);
        if (component == nullptr) {
            return Fail("missing CameraRigMarkerComponent");
        }
        if (ov.fieldName != "Purpose") {
            return Fail("unknown CameraRigMarkerComponent field '" + ov.fieldName + "'");
        }
        component->purpose = ov.value.as<std::string>();
        return Status::Ok();
    }

    return Fail("unsupported component '" + ov.componentName + "'");
}

} // namespace

Status PrefabOverrideSet::Apply(Scene& scene) {
    for (const auto& override : m_Overrides) {
        Status status;
        try {
            status = ApplyOne(scene, override);
        } catch (const std::exception& e) {
            status = Fail(std::string("exception while applying override: ") + e.what());
        }
        if (!status) {
            return status;
        }
    }
    return Status::Ok();
}

} // namespace Hockey
