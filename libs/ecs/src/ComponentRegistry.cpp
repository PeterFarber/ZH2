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

FieldMetadata MakeFloatRangeField(std::string name, std::size_t offset, float minValue, float maxValue,
                                  float speed = 0.1f, std::string displayName = {}) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Float, offset, std::move(displayName));
    field.minFloat = minValue;
    field.maxFloat = maxValue;
    field.speed = speed;
    return field;
}

FieldMetadata MakeVecRangeField(std::string name, FieldType type, std::size_t offset, float minValue, float maxValue,
                                float speed = 0.1f, std::string displayName = {}) {
    FieldMetadata field = MakeField(std::move(name), type, offset, std::move(displayName));
    field.minFloat = minValue;
    field.maxFloat = maxValue;
    field.speed = speed;
    return field;
}

FieldMetadata MakeColorField(std::string name, std::size_t offset, std::string displayName = {}) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Vec3, offset, std::move(displayName));
    field.hint = FieldHint::Color;
    field.minFloat = 0.0f;
    field.maxFloat = 1.0f;
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
        md.name = "ObjectSettingsComponent";
        md.displayName = "Object Settings";
        md.addable = false;
        md.removable = false;
        md.fields.push_back(MakeField("Tag", FieldType::String, offsetof(ObjectSettingsComponent, tag)));
        md.fields.push_back(MakeField("Layer", FieldType::String, offsetof(ObjectSettingsComponent, layer)));
        md.fields.push_back(MakeField("Static", FieldType::Bool, offsetof(ObjectSettingsComponent, isStatic)));
        RegisterComponent<ObjectSettingsComponent>(std::move(md));
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
        // Hierarchy is edited through Scene::SetParent/RemoveParent (which keep
        // the parent/children lists and world transforms consistent), so the
        // inspector shows these read-only rather than letting raw UUID edits
        // break the invariants. ChildrenComponent has no scalar field to expose
        // (it is a UUID vector), so it registers as a presence-only entry.
        ComponentMetadata md;
        md.name = "ParentComponent";
        md.displayName = "Parent";
        md.addable = false;
        md.removable = false;
        FieldMetadata parentField = MakeField("Parent", FieldType::UUID, offsetof(ParentComponent, parentId));
        parentField.readOnly = true;
        md.fields.push_back(parentField);
        RegisterComponent<ParentComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "ChildrenComponent";
        md.displayName = "Children";
        md.addable = false;
        md.removable = false;
        RegisterComponent<ChildrenComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "TeamComponent";
        md.displayName = "Team";
        md.category = "Hockey";
        md.fields.push_back(MakeTeamField("Team", offsetof(TeamComponent, team)));
        RegisterComponent<TeamComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PlayerRoleComponent";
        md.displayName = "Player Role";
        md.category = "Hockey";
        md.fields.push_back(MakeRoleField("Role", offsetof(PlayerRoleComponent, role)));
        RegisterComponent<PlayerRoleComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PuckComponent";
        md.displayName = "Puck";
        md.category = "Hockey";
        md.fields.push_back(MakeField("StartsInPlay", FieldType::Bool, offsetof(PuckComponent, startsInPlay)));
        RegisterComponent<PuckComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "GoalComponent";
        md.displayName = "Goal";
        md.category = "Hockey";
        md.fields.push_back(MakeTeamField("DefendingTeam", offsetof(GoalComponent, defendingTeam)));
        RegisterComponent<GoalComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "SpawnPointComponent";
        md.displayName = "Spawn Point";
        md.category = "Hockey";
        md.fields.push_back(MakeTeamField("Team", offsetof(SpawnPointComponent, team)));
        md.fields.push_back(MakeRoleField("Role", offsetof(SpawnPointComponent, role)));
        md.fields.push_back(MakeField("Index", FieldType::Int, offsetof(SpawnPointComponent, index)));
        md.fields.push_back(
            MakeField("PlayerPrefabPath", FieldType::Path, offsetof(SpawnPointComponent, playerPrefabPath)));
        RegisterComponent<SpawnPointComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "FaceoffSpotComponent";
        md.displayName = "Faceoff Spot";
        md.category = "Hockey";
        md.fields.push_back(MakeField("Index", FieldType::Int, offsetof(FaceoffSpotComponent, index)));
        RegisterComponent<FaceoffSpotComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "RinkComponent";
        md.displayName = "Rink";
        md.category = "Hockey";
        md.fields.push_back(MakeField("RinkName", FieldType::String, offsetof(RinkComponent, rinkName)));
        RegisterComponent<RinkComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PlayAreaComponent";
        md.displayName = "Play Area";
        md.category = "Hockey";
        md.fields.push_back(MakeField("HalfExtents", FieldType::Vec3, offsetof(PlayAreaComponent, halfExtents)));
        RegisterComponent<PlayAreaComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CameraRigMarkerComponent";
        md.displayName = "Camera Rig Marker";
        md.category = "Hockey";
        md.fields.push_back(MakeField("Purpose", FieldType::String, offsetof(CameraRigMarkerComponent, purpose)));
        RegisterComponent<CameraRigMarkerComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CameraComponent";
        md.displayName = "Camera";
        md.category = "Rendering";
        md.fields.push_back(MakeFloatRangeField("FovDegrees", offsetof(CameraComponent, fovDegrees), 1.0f, 179.0f,
                                                0.1f, "FOV"));
        md.fields.push_back(
            MakeFloatRangeField("NearClip", offsetof(CameraComponent, nearClip), 0.001f, 10.0f, 0.01f));
        md.fields.push_back(
            MakeFloatRangeField("FarClip", offsetof(CameraComponent, farClip), 1.0f, 10000.0f, 1.0f));
        md.fields.push_back(MakeField("Primary", FieldType::Bool, offsetof(CameraComponent, primary)));
        RegisterComponent<CameraComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "MeshRendererComponent";
        md.displayName = "Mesh Renderer";
        md.category = "Rendering";
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
        md.category = "Rendering";
        md.fields.push_back(MakeLightTypeField("Type", offsetof(LightComponent, type)));
        md.fields.push_back(MakeColorField("Color", offsetof(LightComponent, color)));
        md.fields.push_back(
            MakeFloatRangeField("Intensity", offsetof(LightComponent, intensity), 0.0f, 16.0f, 0.05f));
        md.fields.push_back(MakeFloatRangeField("Range", offsetof(LightComponent, range), 0.0f, 500.0f, 0.5f));
        md.fields.push_back(
            MakeFloatRangeField("InnerConeDegrees", offsetof(LightComponent, innerConeDegrees), 0.0f, 90.0f, 0.1f));
        md.fields.push_back(
            MakeFloatRangeField("OuterConeDegrees", offsetof(LightComponent, outerConeDegrees), 0.0f, 90.0f, 0.1f));
        md.fields.push_back(MakeField("CastsShadows", FieldType::Bool, offsetof(LightComponent, castsShadows)));
        RegisterComponent<LightComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "EnvironmentComponent";
        md.displayName = "Environment";
        md.category = "Rendering";
        md.fields.push_back(MakeColorField("AmbientColor", offsetof(EnvironmentComponent, ambientColor)));
        md.fields.push_back(
            MakeFloatRangeField("AmbientIntensity", offsetof(EnvironmentComponent, ambientIntensity), 0.0f, 16.0f,
                                0.05f));
        RegisterComponent<EnvironmentComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "ReflectionProbeComponent";
        md.displayName = "Reflection Probe";
        md.category = "Rendering";
        md.fields.push_back(
            MakeFloatRangeField("Radius", offsetof(ReflectionProbeComponent, radius), 0.0f, 1000.0f, 0.5f));
        md.fields.push_back(
            MakeFloatRangeField("Intensity", offsetof(ReflectionProbeComponent, intensity), 0.0f, 16.0f, 0.05f));
        RegisterComponent<ReflectionProbeComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "DecalComponent";
        md.displayName = "Decal";
        md.category = "Rendering";
        md.fields.push_back(MakeField("MaterialName", FieldType::String, offsetof(DecalComponent, materialName)));
        md.fields.push_back(
            MakeVecRangeField("Size", FieldType::Vec3, offsetof(DecalComponent, size), 0.0f, 100.0f, 0.1f));
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
