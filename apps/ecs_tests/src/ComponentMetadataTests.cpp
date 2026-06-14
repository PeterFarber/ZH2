#include "Test.hpp"

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
}
