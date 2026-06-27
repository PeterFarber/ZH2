#include "Test.hpp"

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Prefab.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

void RunPrefabTests() {
    HockeyTest::BeginSuite("PrefabTests");

    Scene source("Source");
    Entity net = source.CreateEntity("Goal Net");
    StickAttachmentComponent attachment;
    attachment.stickPrefabPath = "data/raw/prefabs/Stick_Prefab.prefab.yaml";
    net.AddComponent<StickAttachmentComponent>(attachment);
    Entity trigger = source.CreateEntity("Goal Trigger");
    trigger.AddComponent<GoalComponent>(GoalComponent{Team::Home});
    source.SetParent(trigger, net, false);

    const std::filesystem::path path = Hockey::Paths::TempFile("goalnet.prefab.yaml");

    Prefab prefab;
    HK_CHECK(static_cast<bool>(prefab.SaveFromEntity(source, net, path)));
    HK_CHECK(std::filesystem::exists(path));

    Scene target("Target");
    Result<Entity> result = prefab.Instantiate(target, path);
    HK_CHECK(static_cast<bool>(result));

    Entity root = result.value;
    HK_CHECK(root.IsValid());
    HK_CHECK_EQ(root.GetName(), std::string("Goal Net"));
    HK_CHECK(root.GetUUID() != net.GetUUID());
    HK_CHECK(root.HasComponent<PrefabComponent>());
    HK_CHECK(!root.HasComponent<ParentComponent>());

    HK_CHECK_EQ(target.GetChildren(root).size(), static_cast<std::size_t>(1));
    Entity childInstance = target.GetChildren(root)[0];
    HK_CHECK(childInstance.GetUUID() != trigger.GetUUID());
    HK_CHECK(childInstance.HasComponent<GoalComponent>());
    HK_CHECK(childInstance.HasComponent<PrefabComponent>());
    HK_CHECK(childInstance.HasComponent<ParentComponent>());
    HK_CHECK(childInstance.GetComponent<ParentComponent>().parentId == root.GetUUID());

    // Overrides: transform, name, active, hockey marker.
    PrefabOverrideSet overrides;

    PrefabOverride positionOverride;
    positionOverride.entityId = root.GetUUID();
    positionOverride.componentName = "TransformComponent";
    positionOverride.fieldName = "Position";
    positionOverride.value = YAML::Load("[9.0, 8.0, 7.0]");
    overrides.AddOverride(positionOverride);

    PrefabOverride nameOverride;
    nameOverride.entityId = root.GetUUID();
    nameOverride.componentName = "NameComponent";
    nameOverride.fieldName = "Name";
    nameOverride.value = YAML::Load("Renamed Net");
    overrides.AddOverride(nameOverride);

    PrefabOverride activeOverride;
    activeOverride.entityId = root.GetUUID();
    activeOverride.componentName = "ActiveComponent";
    activeOverride.fieldName = "Active";
    activeOverride.value = YAML::Load("false");
    overrides.AddOverride(activeOverride);

    PrefabOverride settingsOverride;
    settingsOverride.entityId = root.GetUUID();
    settingsOverride.componentName = "ObjectSettingsComponent";
    settingsOverride.fieldName = "Tag";
    settingsOverride.value = YAML::Load("Goal");
    overrides.AddOverride(settingsOverride);

    PrefabOverride markerOverride;
    markerOverride.entityId = childInstance.GetUUID();
    markerOverride.componentName = "GoalComponent";
    markerOverride.fieldName = "DefendingTeam";
    markerOverride.value = YAML::Load("Away");
    overrides.AddOverride(markerOverride);

    PrefabOverride stickPathOverride;
    stickPathOverride.entityId = root.GetUUID();
    stickPathOverride.componentName = "StickAttachmentComponent";
    stickPathOverride.fieldName = "StickPrefabPath";
    stickPathOverride.value = YAML::Load("data/raw/prefabs/Alternate_Stick.prefab.yaml");
    overrides.AddOverride(stickPathOverride);

    HK_CHECK(static_cast<bool>(overrides.Apply(target)));
    HK_CHECK_NEAR(root.GetComponent<TransformComponent>().localPosition.x, 9.0f, 1e-4);
    HK_CHECK_NEAR(root.GetComponent<TransformComponent>().localPosition.z, 7.0f, 1e-4);
    HK_CHECK_EQ(root.GetName(), std::string("Renamed Net"));
    HK_CHECK(!root.IsActive());
    HK_CHECK_EQ(root.GetComponent<ObjectSettingsComponent>().tag, std::string("Goal"));
    HK_CHECK(childInstance.GetComponent<GoalComponent>().defendingTeam == Team::Away);
    HK_CHECK(root.HasComponent<StickAttachmentComponent>());
    if (root.HasComponent<StickAttachmentComponent>()) {
        HK_CHECK_EQ(root.GetComponent<StickAttachmentComponent>().stickPrefabPath.generic_string(),
                    std::string("data/raw/prefabs/Alternate_Stick.prefab.yaml"));
    }
}
