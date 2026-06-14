#include "Hockey/ECS/ComponentRegistry.hpp"

#include <cstddef>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/RenderComponents.hpp"

namespace Hockey {

namespace {

FieldMetadata MakeField(std::string name, FieldType type, std::size_t offset, std::string displayName = {}) {
    FieldMetadata field;
    field.name = std::move(name);
    field.displayName = displayName.empty() ? field.name : std::move(displayName);
    field.type = type;
    field.offset = offset;
    return field;
}

FieldMetadata MakeLightTypeField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"Directional", "Point", "Spot"};
    field.enumValues = {static_cast<int>(LightComponent::Type::Directional),
                        static_cast<int>(LightComponent::Type::Point), static_cast<int>(LightComponent::Type::Spot)};
    return field;
}

FieldMetadata MakeTeamField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"None", "Home", "Away"};
    field.enumValues = {static_cast<int>(Team::None), static_cast<int>(Team::Home), static_cast<int>(Team::Away)};
    return field;
}

FieldMetadata MakeRoleField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"Skater", "Goalie"};
    field.enumValues = {static_cast<int>(PlayerRole::Skater), static_cast<int>(PlayerRole::Goalie)};
    return field;
}

} // namespace

ComponentRegistry& ComponentRegistry::Get() {
    static ComponentRegistry instance;
    return instance;
}

const ComponentMetadata* ComponentRegistry::FindByName(const std::string& name) const {
    for (const auto& component : m_Components) {
        if (component.name == name) {
            return &component;
        }
    }
    return nullptr;
}

const std::vector<ComponentMetadata>& ComponentRegistry::All() const {
    return m_Components;
}

void ComponentRegistry::RegisterPhase2Components() {
    m_Components.clear();

    {
        ComponentMetadata md;
        md.name = "IDComponent";
        md.displayName = "ID";
        md.addable = false;
        md.removable = false;
        FieldMetadata idField = MakeField("Id", FieldType::UUID, offsetof(IDComponent, id), "UUID");
        idField.readOnly = true;
        md.fields.push_back(idField);
        md.add = [](Entity) {};
        md.remove = [](Entity) {};
        RegisterComponent<IDComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "NameComponent";
        md.displayName = "Name";
        md.removable = false;
        md.fields.push_back(MakeField("Name", FieldType::String, offsetof(NameComponent, name)));
        RegisterComponent<NameComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "ActiveComponent";
        md.displayName = "Active";
        md.removable = false;
        md.fields.push_back(MakeField("Active", FieldType::Bool, offsetof(ActiveComponent, active)));
        FieldMetadata inHierarchy =
            MakeField("ActiveInHierarchy", FieldType::Bool, offsetof(ActiveComponent, activeInHierarchy));
        inHierarchy.readOnly = true;
        md.fields.push_back(inHierarchy);
        RegisterComponent<ActiveComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "TransformComponent";
        md.displayName = "Transform";
        md.removable = false;
        md.fields.push_back(MakeField("Position", FieldType::Vec3, offsetof(TransformComponent, localPosition)));
        md.fields.push_back(MakeField("Rotation", FieldType::Vec3, offsetof(TransformComponent, localRotation)));
        md.fields.push_back(MakeField("Scale", FieldType::Vec3, offsetof(TransformComponent, localScale)));
        RegisterComponent<TransformComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "TeamComponent";
        md.displayName = "Team";
        md.fields.push_back(MakeTeamField("Team", offsetof(TeamComponent, team)));
        RegisterComponent<TeamComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PlayerRoleComponent";
        md.displayName = "Player Role";
        md.fields.push_back(MakeRoleField("Role", offsetof(PlayerRoleComponent, role)));
        RegisterComponent<PlayerRoleComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PuckComponent";
        md.displayName = "Puck";
        md.fields.push_back(MakeField("StartsInPlay", FieldType::Bool, offsetof(PuckComponent, startsInPlay)));
        RegisterComponent<PuckComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "GoalComponent";
        md.displayName = "Goal";
        md.fields.push_back(MakeTeamField("DefendingTeam", offsetof(GoalComponent, defendingTeam)));
        RegisterComponent<GoalComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "SpawnPointComponent";
        md.displayName = "Spawn Point";
        md.fields.push_back(MakeTeamField("Team", offsetof(SpawnPointComponent, team)));
        md.fields.push_back(MakeRoleField("Role", offsetof(SpawnPointComponent, role)));
        md.fields.push_back(MakeField("Index", FieldType::Int, offsetof(SpawnPointComponent, index)));
        RegisterComponent<SpawnPointComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "FaceoffSpotComponent";
        md.displayName = "Faceoff Spot";
        md.fields.push_back(MakeField("Index", FieldType::Int, offsetof(FaceoffSpotComponent, index)));
        RegisterComponent<FaceoffSpotComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "RinkComponent";
        md.displayName = "Rink";
        md.fields.push_back(MakeField("RinkName", FieldType::String, offsetof(RinkComponent, rinkName)));
        RegisterComponent<RinkComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PlayAreaComponent";
        md.displayName = "Play Area";
        md.fields.push_back(MakeField("HalfExtents", FieldType::Vec3, offsetof(PlayAreaComponent, halfExtents)));
        RegisterComponent<PlayAreaComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CameraRigMarkerComponent";
        md.displayName = "Camera Rig Marker";
        md.fields.push_back(MakeField("Purpose", FieldType::String, offsetof(CameraRigMarkerComponent, purpose)));
        RegisterComponent<CameraRigMarkerComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CameraComponent";
        md.displayName = "Camera";
        md.fields.push_back(MakeField("FovDegrees", FieldType::Float, offsetof(CameraComponent, fovDegrees)));
        md.fields.push_back(MakeField("NearClip", FieldType::Float, offsetof(CameraComponent, nearClip)));
        md.fields.push_back(MakeField("FarClip", FieldType::Float, offsetof(CameraComponent, farClip)));
        md.fields.push_back(MakeField("Primary", FieldType::Bool, offsetof(CameraComponent, primary)));
        RegisterComponent<CameraComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "MeshRendererComponent";
        md.displayName = "Mesh Renderer";
        FieldMetadata meshAsset =
            MakeField("MeshAsset", FieldType::AssetRef, offsetof(MeshRendererComponent, meshAsset));
        meshAsset.assetTypeName = "Mesh";
        md.fields.push_back(meshAsset);
        FieldMetadata materialAsset =
            MakeField("MaterialAsset", FieldType::AssetRef, offsetof(MeshRendererComponent, materialAsset));
        materialAsset.assetTypeName = "Material";
        md.fields.push_back(materialAsset);
        md.fields.push_back(MakeField("MeshName", FieldType::String, offsetof(MeshRendererComponent, meshName)));
        md.fields.push_back(
            MakeField("MaterialName", FieldType::String, offsetof(MeshRendererComponent, materialName)));
        md.fields.push_back(MakeField("Visible", FieldType::Bool, offsetof(MeshRendererComponent, visible)));
        md.fields.push_back(MakeField("CastsShadows", FieldType::Bool, offsetof(MeshRendererComponent, castsShadows)));
        md.fields.push_back(
            MakeField("ReceivesShadows", FieldType::Bool, offsetof(MeshRendererComponent, receivesShadows)));
        RegisterComponent<MeshRendererComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "LightComponent";
        md.displayName = "Light";
        md.fields.push_back(MakeLightTypeField("Type", offsetof(LightComponent, type)));
        md.fields.push_back(MakeField("Color", FieldType::Vec3, offsetof(LightComponent, color)));
        md.fields.push_back(MakeField("Intensity", FieldType::Float, offsetof(LightComponent, intensity)));
        md.fields.push_back(MakeField("Range", FieldType::Float, offsetof(LightComponent, range)));
        md.fields.push_back(
            MakeField("InnerConeDegrees", FieldType::Float, offsetof(LightComponent, innerConeDegrees)));
        md.fields.push_back(
            MakeField("OuterConeDegrees", FieldType::Float, offsetof(LightComponent, outerConeDegrees)));
        md.fields.push_back(MakeField("CastsShadows", FieldType::Bool, offsetof(LightComponent, castsShadows)));
        RegisterComponent<LightComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "EnvironmentComponent";
        md.displayName = "Environment";
        md.fields.push_back(MakeField("AmbientColor", FieldType::Vec3, offsetof(EnvironmentComponent, ambientColor)));
        md.fields.push_back(
            MakeField("AmbientIntensity", FieldType::Float, offsetof(EnvironmentComponent, ambientIntensity)));
        RegisterComponent<EnvironmentComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "ReflectionProbeComponent";
        md.displayName = "Reflection Probe";
        md.fields.push_back(MakeField("Radius", FieldType::Float, offsetof(ReflectionProbeComponent, radius)));
        md.fields.push_back(MakeField("Intensity", FieldType::Float, offsetof(ReflectionProbeComponent, intensity)));
        RegisterComponent<ReflectionProbeComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "DecalComponent";
        md.displayName = "Decal";
        md.fields.push_back(MakeField("MaterialName", FieldType::String, offsetof(DecalComponent, materialName)));
        md.fields.push_back(MakeField("Size", FieldType::Vec3, offsetof(DecalComponent, size)));
        md.fields.push_back(MakeField("AffectsBaseColor", FieldType::Bool, offsetof(DecalComponent, affectsBaseColor)));
        md.fields.push_back(MakeField("AffectsNormals", FieldType::Bool, offsetof(DecalComponent, affectsNormals)));
        RegisterComponent<DecalComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PrefabComponent";
        md.displayName = "Prefab";
        md.addable = false;
        FieldMetadata assetId = MakeField("PrefabAssetId", FieldType::UUID, offsetof(PrefabComponent, prefabAssetId));
        assetId.readOnly = true;
        FieldMetadata sourceId =
            MakeField("SourceEntityId", FieldType::UUID, offsetof(PrefabComponent, sourceEntityId));
        sourceId.readOnly = true;
        FieldMetadata sourcePath = MakeField("SourcePath", FieldType::Path, offsetof(PrefabComponent, sourcePath));
        sourcePath.readOnly = true;
        md.fields.push_back(assetId);
        md.fields.push_back(sourceId);
        md.fields.push_back(sourcePath);
        RegisterComponent<PrefabComponent>(std::move(md));
    }
}

} // namespace Hockey
