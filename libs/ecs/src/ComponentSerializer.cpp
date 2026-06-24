#include "Hockey/ECS/ComponentSerializer.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/YAML.hpp"

namespace Hockey {

namespace {

struct ExternalHook {
    ComponentSerializer::SerializeFn serialize;
    ComponentSerializer::DeserializeFn deserialize;
};

std::vector<ExternalHook>& ExternalHooks() {
    static std::vector<ExternalHook> hooks;
    return hooks;
}

void SerializeHockeyMarkers(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<TeamComponent>()) {
        out << YAML::Key << "TeamComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Team" << YAML::Value << TeamToString(entity.GetComponent<TeamComponent>().team);
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PlayerRoleComponent>()) {
        out << YAML::Key << "PlayerRoleComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Role" << YAML::Value
            << PlayerRoleToString(entity.GetComponent<PlayerRoleComponent>().role);
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PuckComponent>()) {
        out << YAML::Key << "PuckComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "StartsInPlay" << YAML::Value << entity.GetComponent<PuckComponent>().startsInPlay;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<GoalComponent>()) {
        out << YAML::Key << "GoalComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "DefendingTeam" << YAML::Value
            << TeamToString(entity.GetComponent<GoalComponent>().defendingTeam);
        out << YAML::EndMap;
    }
    if (entity.HasComponent<SpawnPointComponent>()) {
        const auto& spawn = entity.GetComponent<SpawnPointComponent>();
        out << YAML::Key << "SpawnPointComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Team" << YAML::Value << TeamToString(spawn.team);
        out << YAML::Key << "FaceoffSpawn" << YAML::Value << spawn.faceoffSpawn;
        if (!spawn.playerPrefabPath.empty()) {
            out << YAML::Key << "PlayerPrefabPath" << YAML::Value << spawn.playerPrefabPath.generic_string();
        }
        out << YAML::EndMap;
    }
    if (entity.HasComponent<RinkComponent>()) {
        out << YAML::Key << "RinkComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "RinkName" << YAML::Value << entity.GetComponent<RinkComponent>().rinkName;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<PlayAreaComponent>()) {
        out << YAML::Key << "PlayAreaComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "HalfExtents" << YAML::Value << entity.GetComponent<PlayAreaComponent>().halfExtents;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<CameraRigMarkerComponent>()) {
        out << YAML::Key << "CameraRigMarkerComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Purpose" << YAML::Value << entity.GetComponent<CameraRigMarkerComponent>().purpose;
        out << YAML::EndMap;
    }
}

void SerializeRenderComponents(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<CameraComponent>()) {
        const auto& camera = entity.GetComponent<CameraComponent>();
        out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "FovDegrees" << YAML::Value << camera.fovDegrees;
        out << YAML::Key << "NearClip" << YAML::Value << camera.nearClip;
        out << YAML::Key << "FarClip" << YAML::Value << camera.farClip;
        out << YAML::Key << "Primary" << YAML::Value << camera.primary;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<MeshRendererComponent>()) {
        const auto& mesh = entity.GetComponent<MeshRendererComponent>();
        out << YAML::Key << "MeshRendererComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MeshAsset" << YAML::Value << mesh.meshAsset;
        out << YAML::Key << "MaterialAsset" << YAML::Value << mesh.materialAsset;
        out << YAML::Key << "MeshName" << YAML::Value << mesh.meshName;
        out << YAML::Key << "MaterialName" << YAML::Value << mesh.materialName;
        out << YAML::Key << "Visible" << YAML::Value << mesh.visible;
        out << YAML::Key << "CastsShadows" << YAML::Value << mesh.castsShadows;
        out << YAML::Key << "ReceivesShadows" << YAML::Value << mesh.receivesShadows;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<LightComponent>()) {
        const auto& light = entity.GetComponent<LightComponent>();
        out << YAML::Key << "LightComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Type" << YAML::Value << LightTypeToString(light.type);
        out << YAML::Key << "Color" << YAML::Value << light.color;
        out << YAML::Key << "Intensity" << YAML::Value << light.intensity;
        out << YAML::Key << "Range" << YAML::Value << light.range;
        out << YAML::Key << "InnerConeDegrees" << YAML::Value << light.innerConeDegrees;
        out << YAML::Key << "OuterConeDegrees" << YAML::Value << light.outerConeDegrees;
        out << YAML::Key << "CastsShadows" << YAML::Value << light.castsShadows;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<EnvironmentComponent>()) {
        const auto& env = entity.GetComponent<EnvironmentComponent>();
        out << YAML::Key << "EnvironmentComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AmbientColor" << YAML::Value << env.ambientColor;
        out << YAML::Key << "AmbientIntensity" << YAML::Value << env.ambientIntensity;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<ReflectionProbeComponent>()) {
        const auto& probe = entity.GetComponent<ReflectionProbeComponent>();
        out << YAML::Key << "ReflectionProbeComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Radius" << YAML::Value << probe.radius;
        out << YAML::Key << "Intensity" << YAML::Value << probe.intensity;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<DecalComponent>()) {
        const auto& decal = entity.GetComponent<DecalComponent>();
        out << YAML::Key << "DecalComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MaterialName" << YAML::Value << decal.materialName;
        out << YAML::Key << "Size" << YAML::Value << decal.size;
        out << YAML::Key << "AffectsBaseColor" << YAML::Value << decal.affectsBaseColor;
        out << YAML::Key << "AffectsNormals" << YAML::Value << decal.affectsNormals;
        out << YAML::EndMap;
    }
}

} // namespace

void ComponentSerializer::SerializeEntity(YAML::Emitter& out, Entity entity) {
    out << YAML::BeginMap;
    out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID().Value();

    out << YAML::Key << "NameComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << entity.GetComponent<NameComponent>().name;
    out << YAML::EndMap;

    out << YAML::Key << "ObjectSettingsComponent" << YAML::Value << YAML::BeginMap;
    const auto& settings = entity.GetComponent<ObjectSettingsComponent>();
    out << YAML::Key << "Tag" << YAML::Value << settings.tag;
    out << YAML::Key << "Layer" << YAML::Value << settings.layer;
    out << YAML::Key << "Static" << YAML::Value << settings.isStatic;
    out << YAML::EndMap;

    out << YAML::Key << "ActiveComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Active" << YAML::Value << entity.GetComponent<ActiveComponent>().active;
    out << YAML::EndMap;

    {
        const auto& transform = entity.GetComponent<TransformComponent>();
        out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Position" << YAML::Value << transform.localPosition;
        out << YAML::Key << "Rotation" << YAML::Value << transform.localRotation;
        out << YAML::Key << "Scale" << YAML::Value << transform.localScale;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<ParentComponent>()) {
        out << YAML::Key << "ParentComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Parent" << YAML::Value << entity.GetComponent<ParentComponent>().parentId.Value();
        out << YAML::EndMap;
    }

    {
        const auto& children = entity.GetComponent<ChildrenComponent>();
        out << YAML::Key << "ChildrenComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
        for (const UUID childId : children.children) {
            out << childId.Value();
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<PrefabComponent>()) {
        const auto& prefab = entity.GetComponent<PrefabComponent>();
        out << YAML::Key << "PrefabComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "PrefabAssetId" << YAML::Value << prefab.prefabAssetId.Value();
        out << YAML::Key << "SourceEntityId" << YAML::Value << prefab.sourceEntityId.Value();
        out << YAML::Key << "SourcePath" << YAML::Value << prefab.sourcePath.generic_string();
        out << YAML::EndMap;
    }

    SerializeHockeyMarkers(out, entity);
    SerializeRenderComponents(out, entity);

    for (const ExternalHook& hook : ExternalHooks()) {
        if (hook.serialize) {
            hook.serialize(out, entity);
        }
    }

    out << YAML::EndMap;
}

Entity ComponentSerializer::DeserializeEntity(Scene& scene, const YAML::Node& node) {
    const UUID id(node["Entity"].as<std::uint64_t>());

    std::string name = "GameObject";
    if (const auto nameNode = node["NameComponent"]; nameNode && nameNode["Name"]) {
        name = nameNode["Name"].as<std::string>();
    }

    Entity entity = scene.CreateEntityWithUUID(id, name);
    DeserializeCoreComponents(entity, node);
    DeserializeHockeyMarkerComponents(entity, node);
    DeserializeRenderComponents(entity, node);
    DeserializeExternalComponents(entity, node);
    return entity;
}

bool ComponentSerializer::DeserializeExternalComponents(Entity entity, const YAML::Node& node) {
    for (const ExternalHook& hook : ExternalHooks()) {
        if (hook.deserialize) {
            hook.deserialize(entity, node);
        }
    }
    return true;
}

void ComponentSerializer::RegisterExternal(SerializeFn serialize, DeserializeFn deserialize) {
    ExternalHooks().push_back(ExternalHook{std::move(serialize), std::move(deserialize)});
}

void ComponentSerializer::ClearExternal() {
    ExternalHooks().clear();
}

bool ComponentSerializer::DeserializeCoreComponents(Entity entity, const YAML::Node& node) {
    entt::registry& registry = entity.GetScene()->Registry();
    const entt::entity handle = entity.Handle();

    if (const auto nameNode = node["NameComponent"]; nameNode && nameNode["Name"]) {
        registry.get<NameComponent>(handle).name = nameNode["Name"].as<std::string>();
    }

    if (const auto settingsNode = node["ObjectSettingsComponent"]) {
        auto& settings = registry.get<ObjectSettingsComponent>(handle);
        if (settingsNode["Tag"]) {
            settings.tag = settingsNode["Tag"].as<std::string>();
        }
        if (settingsNode["Layer"]) {
            settings.layer = settingsNode["Layer"].as<std::string>();
        }
        if (settingsNode["Static"]) {
            settings.isStatic = settingsNode["Static"].as<bool>();
        }
    }

    if (const auto activeNode = node["ActiveComponent"]; activeNode && activeNode["Active"]) {
        registry.get<ActiveComponent>(handle).active = activeNode["Active"].as<bool>();
    }

    if (const auto transformNode = node["TransformComponent"]) {
        auto& transform = registry.get<TransformComponent>(handle);
        ReadVec3(transformNode["Position"], transform.localPosition);
        ReadVec3(transformNode["Rotation"], transform.localRotation);
        ReadVec3(transformNode["Scale"], transform.localScale);
    }

    if (const auto parentNode = node["ParentComponent"]; parentNode && parentNode["Parent"]) {
        registry.emplace_or_replace<ParentComponent>(handle,
                                                     ParentComponent{UUID(parentNode["Parent"].as<std::uint64_t>())});
    }

    if (const auto childrenNode = node["ChildrenComponent"]; childrenNode && childrenNode["Children"]) {
        auto& children = registry.get<ChildrenComponent>(handle).children;
        children.clear();
        for (const auto child : childrenNode["Children"]) {
            children.push_back(UUID(child.as<std::uint64_t>()));
        }
    }

    if (const auto prefabNode = node["PrefabComponent"]) {
        PrefabComponent prefab;
        if (prefabNode["PrefabAssetId"]) {
            prefab.prefabAssetId = UUID(prefabNode["PrefabAssetId"].as<std::uint64_t>());
        }
        if (prefabNode["SourceEntityId"]) {
            prefab.sourceEntityId = UUID(prefabNode["SourceEntityId"].as<std::uint64_t>());
        }
        if (prefabNode["SourcePath"]) {
            prefab.sourcePath = prefabNode["SourcePath"].as<std::string>();
        }
        registry.emplace_or_replace<PrefabComponent>(handle, prefab);
    }

    return true;
}

bool ComponentSerializer::DeserializeHockeyMarkerComponents(Entity entity, const YAML::Node& node) {
    entt::registry& registry = entity.GetScene()->Registry();
    const entt::entity handle = entity.Handle();

    if (const auto teamNode = node["TeamComponent"]) {
        TeamComponent component;
        if (teamNode["Team"]) {
            component.team = TeamFromString(teamNode["Team"].as<std::string>());
        }
        registry.emplace_or_replace<TeamComponent>(handle, component);
    }

    if (const auto roleNode = node["PlayerRoleComponent"]) {
        PlayerRoleComponent component;
        if (roleNode["Role"]) {
            component.role = PlayerRoleFromString(roleNode["Role"].as<std::string>());
        }
        registry.emplace_or_replace<PlayerRoleComponent>(handle, component);
    }

    if (const auto puckNode = node["PuckComponent"]) {
        PuckComponent component;
        if (puckNode["StartsInPlay"]) {
            component.startsInPlay = puckNode["StartsInPlay"].as<bool>();
        }
        registry.emplace_or_replace<PuckComponent>(handle, component);
    }

    if (const auto goalNode = node["GoalComponent"]) {
        GoalComponent component;
        if (goalNode["DefendingTeam"]) {
            component.defendingTeam = TeamFromString(goalNode["DefendingTeam"].as<std::string>());
        }
        registry.emplace_or_replace<GoalComponent>(handle, component);
    }

    if (const auto spawnNode = node["SpawnPointComponent"]) {
        SpawnPointComponent component;
        if (spawnNode["Team"]) {
            component.team = TeamFromString(spawnNode["Team"].as<std::string>());
        }
        if (spawnNode["FaceoffSpawn"]) {
            component.faceoffSpawn = spawnNode["FaceoffSpawn"].as<bool>();
        }
        if (spawnNode["PlayerPrefabPath"]) {
            component.playerPrefabPath = spawnNode["PlayerPrefabPath"].as<std::string>();
        }
        registry.emplace_or_replace<SpawnPointComponent>(handle, component);
    }

    if (const auto rinkNode = node["RinkComponent"]) {
        RinkComponent component;
        if (rinkNode["RinkName"]) {
            component.rinkName = rinkNode["RinkName"].as<std::string>();
        }
        registry.emplace_or_replace<RinkComponent>(handle, component);
    }

    if (const auto playAreaNode = node["PlayAreaComponent"]) {
        PlayAreaComponent component;
        ReadVec3(playAreaNode["HalfExtents"], component.halfExtents);
        registry.emplace_or_replace<PlayAreaComponent>(handle, component);
    }

    if (const auto cameraNode = node["CameraRigMarkerComponent"]) {
        CameraRigMarkerComponent component;
        if (cameraNode["Purpose"]) {
            component.purpose = cameraNode["Purpose"].as<std::string>();
        }
        registry.emplace_or_replace<CameraRigMarkerComponent>(handle, component);
    }

    return true;
}

bool ComponentSerializer::DeserializeRenderComponents(Entity entity, const YAML::Node& node) {
    entt::registry& registry = entity.GetScene()->Registry();
    const entt::entity handle = entity.Handle();

    if (const auto cameraNode = node["CameraComponent"]) {
        CameraComponent component;
        if (cameraNode["FovDegrees"]) {
            component.fovDegrees = cameraNode["FovDegrees"].as<float>();
        }
        if (cameraNode["NearClip"]) {
            component.nearClip = cameraNode["NearClip"].as<float>();
        }
        if (cameraNode["FarClip"]) {
            component.farClip = cameraNode["FarClip"].as<float>();
        }
        if (cameraNode["Primary"]) {
            component.primary = cameraNode["Primary"].as<bool>();
        }
        registry.emplace_or_replace<CameraComponent>(handle, component);
    }

    if (const auto meshNode = node["MeshRendererComponent"]) {
        MeshRendererComponent component;
        if (meshNode["MeshAsset"]) {
            component.meshAsset = meshNode["MeshAsset"].as<std::uint64_t>();
        }
        if (meshNode["MaterialAsset"]) {
            component.materialAsset = meshNode["MaterialAsset"].as<std::uint64_t>();
        }
        if (meshNode["MeshName"]) {
            component.meshName = meshNode["MeshName"].as<std::string>();
        }
        if (meshNode["MaterialName"]) {
            component.materialName = meshNode["MaterialName"].as<std::string>();
        }
        if (meshNode["Visible"]) {
            component.visible = meshNode["Visible"].as<bool>();
        }
        if (meshNode["CastsShadows"]) {
            component.castsShadows = meshNode["CastsShadows"].as<bool>();
        }
        if (meshNode["ReceivesShadows"]) {
            component.receivesShadows = meshNode["ReceivesShadows"].as<bool>();
        }
        registry.emplace_or_replace<MeshRendererComponent>(handle, component);
    }

    if (const auto lightNode = node["LightComponent"]) {
        LightComponent component;
        if (lightNode["Type"]) {
            component.type = LightTypeFromString(lightNode["Type"].as<std::string>());
        }
        ReadVec3(lightNode["Color"], component.color);
        if (lightNode["Intensity"]) {
            component.intensity = lightNode["Intensity"].as<float>();
        }
        if (lightNode["Range"]) {
            component.range = lightNode["Range"].as<float>();
        }
        if (lightNode["InnerConeDegrees"]) {
            component.innerConeDegrees = lightNode["InnerConeDegrees"].as<float>();
        }
        if (lightNode["OuterConeDegrees"]) {
            component.outerConeDegrees = lightNode["OuterConeDegrees"].as<float>();
        }
        if (lightNode["CastsShadows"]) {
            component.castsShadows = lightNode["CastsShadows"].as<bool>();
        }
        registry.emplace_or_replace<LightComponent>(handle, component);
    }

    if (const auto envNode = node["EnvironmentComponent"]) {
        EnvironmentComponent component;
        ReadVec3(envNode["AmbientColor"], component.ambientColor);
        if (envNode["AmbientIntensity"]) {
            component.ambientIntensity = envNode["AmbientIntensity"].as<float>();
        }
        registry.emplace_or_replace<EnvironmentComponent>(handle, component);
    }

    if (const auto probeNode = node["ReflectionProbeComponent"]) {
        ReflectionProbeComponent component;
        if (probeNode["Radius"]) {
            component.radius = probeNode["Radius"].as<float>();
        }
        if (probeNode["Intensity"]) {
            component.intensity = probeNode["Intensity"].as<float>();
        }
        registry.emplace_or_replace<ReflectionProbeComponent>(handle, component);
    }

    if (const auto decalNode = node["DecalComponent"]) {
        DecalComponent component;
        if (decalNode["MaterialName"]) {
            component.materialName = decalNode["MaterialName"].as<std::string>();
        }
        ReadVec3(decalNode["Size"], component.size);
        if (decalNode["AffectsBaseColor"]) {
            component.affectsBaseColor = decalNode["AffectsBaseColor"].as<bool>();
        }
        if (decalNode["AffectsNormals"]) {
            component.affectsNormals = decalNode["AffectsNormals"].as<bool>();
        }
        registry.emplace_or_replace<DecalComponent>(handle, component);
    }

    return true;
}

} // namespace Hockey
