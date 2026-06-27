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

void PrefabOverrideSet::Clear() {
    m_Overrides.clear();
}

bool PrefabOverrideSet::Empty() const {
    return m_Overrides.empty();
}

void PrefabOverrideSet::Serialize(YAML::Emitter& out) const {
    out << YAML::BeginSeq;
    for (const PrefabOverride& ov : m_Overrides) {
        out << YAML::BeginMap;
        out << YAML::Key << "Entity" << YAML::Value << ov.entityId.Value();
        out << YAML::Key << "Component" << YAML::Value << ov.componentName;
        out << YAML::Key << "Field" << YAML::Value << ov.fieldName;
        out << YAML::Key << "Value" << YAML::Value << ov.value;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
}

void PrefabOverrideSet::Deserialize(const YAML::Node& node) {
    m_Overrides.clear();
    if (!node || !node.IsSequence()) {
        return;
    }
    for (const auto& item : node) {
        if (!item["Entity"] || !item["Component"] || !item["Field"]) {
            continue;
        }
        PrefabOverride ov;
        ov.entityId = UUID(item["Entity"].as<std::uint64_t>());
        ov.componentName = item["Component"].as<std::string>();
        ov.fieldName = item["Field"].as<std::string>();
        ov.value = item["Value"];
        m_Overrides.push_back(std::move(ov));
    }
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

    if (ov.componentName == "ObjectSettingsComponent") {
        auto& settings = registry.get<ObjectSettingsComponent>(handle);
        if (ov.fieldName == "Tag") {
            settings.tag = ov.value.as<std::string>();
        } else if (ov.fieldName == "Layer") {
            settings.layer = ov.value.as<std::string>();
        } else if (ov.fieldName == "Static") {
            settings.isStatic = ov.value.as<bool>();
        } else {
            return Fail("unknown ObjectSettingsComponent field '" + ov.fieldName + "'");
        }
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
        } else if (ov.fieldName == "FaceoffSpawn") {
            component->faceoffSpawn = ov.value.as<bool>();
        } else if (ov.fieldName == "PlayerPrefabPath") {
            component->playerPrefabPath = ov.value.as<std::string>();
        } else {
            return Fail("unknown SpawnPointComponent field '" + ov.fieldName + "'");
        }
        return Status::Ok();
    }

    if (ov.componentName == "StickAttachmentComponent") {
        auto* component = registry.try_get<StickAttachmentComponent>(handle);
        if (component == nullptr) {
            return Fail("missing StickAttachmentComponent");
        }
        if (ov.fieldName == "StickEntity") {
            component->stickEntityId = UUID(ov.value.as<std::uint64_t>());
        } else {
            return Fail("unknown StickAttachmentComponent field '" + ov.fieldName + "'");
        }
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
