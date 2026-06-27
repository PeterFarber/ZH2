#include "Test.hpp"

#include <type_traits>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

namespace {

bool HasField(const ComponentMetadata& md, const std::string& name) {
    for (const auto& field : md.fields) {
        if (field.name == name) {
            return true;
        }
    }
    return false;
}

const FieldMetadata* FindField(const ComponentMetadata& md, const std::string& name) {
    for (const auto& field : md.fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

template <typename T, typename = void> struct HasSpawnRoleField : std::false_type {};
template <typename T> struct HasSpawnRoleField<T, std::void_t<decltype(&T::role)>> : std::true_type {};

template <typename T, typename = void> struct HasSpawnIndexField : std::false_type {};
template <typename T> struct HasSpawnIndexField<T, std::void_t<decltype(&T::index)>> : std::true_type {};

template <typename T, typename = void> struct HasFaceoffSpawnField : std::false_type {};
template <typename T>
struct HasFaceoffSpawnField<T, std::void_t<decltype(&T::faceoffSpawn)>> : std::true_type {};

} // namespace

void RunComponentMetadataTests() {
    HockeyTest::BeginSuite("ComponentMetadataTests");

    ComponentRegistry& registry = ComponentRegistry::Get();
    registry.RegisterPhase2Components();

    HK_CHECK(!registry.All().empty());

    const ComponentMetadata* transform = registry.FindByName("TransformComponent");
    HK_CHECK(transform != nullptr);
    if (transform != nullptr) {
        HK_CHECK(HasField(*transform, "Position"));
        HK_CHECK(HasField(*transform, "Rotation"));
        HK_CHECK(HasField(*transform, "Scale"));
    }

    const ComponentMetadata* team = registry.FindByName("TeamComponent");
    HK_CHECK(team != nullptr);
    if (team != nullptr) {
        HK_CHECK(!team->fields.empty());
        HK_CHECK(team->fields[0].type == FieldType::Enum);
        HK_CHECK(!team->fields[0].enumNames.empty());
    }

    const ComponentMetadata* role = registry.FindByName("PlayerRoleComponent");
    HK_CHECK(role != nullptr);
    if (role != nullptr) {
        HK_CHECK(!role->fields.empty());
        HK_CHECK(role->fields[0].type == FieldType::Enum);
    }

    const ComponentMetadata* objectSettings = registry.FindByName("ObjectSettingsComponent");
    HK_CHECK(objectSettings != nullptr);
    if (objectSettings != nullptr) {
        HK_CHECK(!objectSettings->addable);
        HK_CHECK(!objectSettings->removable);
        HK_CHECK(HasField(*objectSettings, "Tag"));
        HK_CHECK(HasField(*objectSettings, "Layer"));
        HK_CHECK(HasField(*objectSettings, "Static"));
    }

    const ComponentMetadata* spawn = registry.FindByName("SpawnPointComponent");
    HK_CHECK(spawn != nullptr);
    if (spawn != nullptr) {
        HK_CHECK_EQ(spawn->category, std::string("Hockey"));
        HK_CHECK(HasField(*spawn, "Team"));
        HK_CHECK(HasField(*spawn, "FaceoffSpawn"));
        HK_CHECK(HasField(*spawn, "PlayerPrefabPath"));
        HK_CHECK(!HasField(*spawn, "Role"));
        HK_CHECK(!HasField(*spawn, "Index"));
    }
    HK_CHECK(!HasSpawnRoleField<SpawnPointComponent>::value);
    HK_CHECK(!HasSpawnIndexField<SpawnPointComponent>::value);
    HK_CHECK(HasFaceoffSpawnField<SpawnPointComponent>::value);
    HK_CHECK(registry.FindByName("FaceoffSpotComponent") == nullptr);

    HK_CHECK(registry.FindByName("StickAttachmentComponent") == nullptr);

    const ComponentMetadata* light = registry.FindByName("LightComponent");
    HK_CHECK(light != nullptr);
    if (light != nullptr) {
        HK_CHECK_EQ(light->category, std::string("Rendering"));
        const FieldMetadata* color = FindField(*light, "Color");
        HK_CHECK(color != nullptr && color->hint == FieldHint::Color);
        const FieldMetadata* intensity = FindField(*light, "Intensity");
        HK_CHECK(intensity != nullptr && intensity->maxFloat > intensity->minFloat);
    }

    const ComponentMetadata* camera = registry.FindByName("CameraComponent");
    HK_CHECK(camera != nullptr);
    if (camera != nullptr) {
        const FieldMetadata* fov = FindField(*camera, "FovDegrees");
        HK_CHECK(fov != nullptr && fov->minFloat > 0.0f && fov->maxFloat > fov->minFloat);
    }

    // has/add/remove callbacks.
    Scene scene("Test");
    Entity e = scene.CreateEntity("E");
    const ComponentMetadata* puck = registry.FindByName("PuckComponent");
    HK_CHECK(puck != nullptr);
    if (puck != nullptr) {
        HK_CHECK(!puck->has(e));
        puck->add(e);
        HK_CHECK(puck->has(e));
        HK_CHECK(e.HasComponent<PuckComponent>());
        puck->remove(e);
        HK_CHECK(!puck->has(e));
    }

    // Read-only ID metadata, not addable/removable.
    const ComponentMetadata* id = registry.FindByName("IDComponent");
    HK_CHECK(id != nullptr);
    if (id != nullptr) {
        HK_CHECK(!id->fields.empty());
        HK_CHECK(id->fields[0].readOnly);
        HK_CHECK(!id->addable);
        HK_CHECK(!id->removable);
    }

    // Hierarchy components are registered read-only and not user-addable.
    const ComponentMetadata* parent = registry.FindByName("ParentComponent");
    HK_CHECK(parent != nullptr);
    if (parent != nullptr) {
        HK_CHECK(!parent->addable);
        HK_CHECK(!parent->removable);
        HK_CHECK(HasField(*parent, "Parent"));
        HK_CHECK(!parent->fields.empty() && parent->fields[0].readOnly);
    }

    const ComponentMetadata* children = registry.FindByName("ChildrenComponent");
    HK_CHECK(children != nullptr);
    if (children != nullptr) {
        HK_CHECK(!children->addable);
        HK_CHECK(!children->removable);
    }

    // AssetRef field type now stringifies (previously fell through to Unknown).
    HK_CHECK_EQ(std::string(FieldTypeToString(FieldType::AssetRef)), std::string("AssetRef"));
}
