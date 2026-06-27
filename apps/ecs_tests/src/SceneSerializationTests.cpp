#include "Test.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"

using namespace Hockey;

namespace {

bool ChildrenContains(Scene& scene, Entity parent, UUID childId) {
    for (Entity e : scene.GetChildren(parent)) {
        if (e.GetUUID() == childId) {
            return true;
        }
    }
    return false;
}

std::vector<UUID> EntityIds(const std::vector<Entity>& entities) {
    std::vector<UUID> ids;
    ids.reserve(entities.size());
    for (const Entity& entity : entities) {
        ids.push_back(entity.GetUUID());
    }
    return ids;
}

} // namespace

void RunSceneSerializationTests() {
    HockeyTest::BeginSuite("SceneSerializationTests");

    Scene scene("Main Rink");
    scene.SetMode(SceneMode::Server);

    Entity rink = scene.CreateEntity("Rink");
    rink.AddComponent<RinkComponent>(RinkComponent{"Test Rink"});
    rink.GetComponent<TransformComponent>().localPosition = glm::vec3(1.0f, 2.0f, 3.0f);
    auto& rinkSettings = rink.GetComponent<ObjectSettingsComponent>();
    rinkSettings.tag = "Rink";
    rinkSettings.layer = "Gameplay";
    rinkSettings.isStatic = true;

    Entity puck = scene.CreateEntity("Puck Spawn");
    puck.AddComponent<PuckComponent>(PuckComponent{true});
    puck.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 0.05f, 0.0f);
    scene.SetParent(puck, rink, false);
    puck.SetActive(false);

    Entity goal = scene.CreateEntity("Home Goal");
    goal.AddComponent<GoalComponent>(GoalComponent{Team::Home});

    Entity prefabInstance = scene.CreateEntity("PrefabInstance");
    prefabInstance.AddComponent<PrefabComponent>(
        PrefabComponent{UUID(111ULL), UUID(222ULL), "data/raw/prefabs/x.prefab.yaml"});

    const UUID rinkId = rink.GetUUID();
    const UUID puckId = puck.GetUUID();
    const UUID goalId = goal.GetUUID();
    const UUID prefabId = prefabInstance.GetUUID();

    const std::filesystem::path path = Hockey::Paths::TempFile("roundtrip.scene.yaml");

    SceneSerializer serializer(scene);
    HK_CHECK(static_cast<bool>(serializer.Serialize(path)));
    HK_CHECK(std::filesystem::exists(path));

    Scene loaded("Empty");
    SceneSerializer deserializer(loaded);
    HK_CHECK(static_cast<bool>(deserializer.Deserialize(path)));

    HK_CHECK_EQ(loaded.GetName(), std::string("Main Rink"));
    HK_CHECK(loaded.GetMode() == SceneMode::Server);
    HK_CHECK_EQ(loaded.EntityCount(), static_cast<std::size_t>(4));

    // Scene mode token helpers round-trip, and unknown tokens fall back to Edit.
    HK_CHECK_EQ(std::string(SceneModeToString(SceneMode::ClientPrediction)), std::string("ClientPrediction"));
    HK_CHECK(SceneModeFromString("Play") == SceneMode::Play);
    HK_CHECK(SceneModeFromString("nonsense") == SceneMode::Edit);

    Entity lr = loaded.FindEntityByUUID(rinkId);
    HK_CHECK(lr.IsValid());
    HK_CHECK_EQ(lr.GetName(), std::string("Rink"));
    HK_CHECK(lr.HasComponent<RinkComponent>());
    HK_CHECK_EQ(lr.GetComponent<RinkComponent>().rinkName, std::string("Test Rink"));
    HK_CHECK(lr.HasComponent<ObjectSettingsComponent>());
    HK_CHECK_EQ(lr.GetComponent<ObjectSettingsComponent>().tag, std::string("Rink"));
    HK_CHECK_EQ(lr.GetComponent<ObjectSettingsComponent>().layer, std::string("Gameplay"));
    HK_CHECK(lr.GetComponent<ObjectSettingsComponent>().isStatic);
    HK_CHECK_NEAR(lr.GetComponent<TransformComponent>().localPosition.x, 1.0f, 1e-4);
    HK_CHECK_NEAR(lr.GetComponent<TransformComponent>().localPosition.z, 3.0f, 1e-4);

    Entity lp = loaded.FindEntityByUUID(puckId);
    HK_CHECK(lp.IsValid());
    HK_CHECK(lp.HasComponent<PuckComponent>());
    HK_CHECK(lp.GetComponent<PuckComponent>().startsInPlay);
    HK_CHECK(lp.HasComponent<ParentComponent>());
    HK_CHECK(lp.GetComponent<ParentComponent>().parentId == rinkId);
    HK_CHECK(!lp.IsActive());
    HK_CHECK(ChildrenContains(loaded, lr, puckId));

    Entity lg = loaded.FindEntityByUUID(goalId);
    HK_CHECK(lg.IsValid());
    HK_CHECK(lg.HasComponent<GoalComponent>());
    HK_CHECK(lg.GetComponent<GoalComponent>().defendingTeam == Team::Home);

    Entity lpf = loaded.FindEntityByUUID(prefabId);
    HK_CHECK(lpf.IsValid());
    HK_CHECK(lpf.HasComponent<PrefabComponent>());
    HK_CHECK(lpf.GetComponent<PrefabComponent>().prefabAssetId == UUID(111ULL));
    HK_CHECK(lpf.GetComponent<PrefabComponent>().sourceEntityId == UUID(222ULL));

    // Root and child order round-trip through scene serialization.
    {
        Scene ordered("Ordered Scene");
        Entity a = ordered.CreateEntity("A");
        Entity b = ordered.CreateEntity("B");
        Entity parent = ordered.CreateEntity("Parent");
        Entity c = ordered.CreateEntity("C");
        Entity childA = ordered.CreateEntity("ChildA");
        Entity childB = ordered.CreateEntity("ChildB");
        ordered.SetParent(childA, parent, false);
        ordered.SetParent(childB, parent, false);
        ordered.MoveEntity(c, Entity{}, 1, false);
        ordered.MoveEntity(childB, parent, 0, false);

        const std::filesystem::path orderedPath = Hockey::Paths::TempFile("ordered_roundtrip.scene.yaml");
        HK_CHECK(static_cast<bool>(SceneSerializer(ordered).Serialize(orderedPath)));

        Scene loadedOrdered("Empty");
        HK_CHECK(static_cast<bool>(SceneSerializer(loadedOrdered).Deserialize(orderedPath)));

        const std::vector<UUID> roots = EntityIds(loadedOrdered.GetRootEntities());
        HK_CHECK_EQ(roots.size(), static_cast<std::size_t>(4));
        HK_CHECK(roots[0] == a.GetUUID());
        HK_CHECK(roots[1] == c.GetUUID());
        HK_CHECK(roots[2] == b.GetUUID());
        HK_CHECK(roots[3] == parent.GetUUID());

        Entity loadedParent = loadedOrdered.FindEntityByUUID(parent.GetUUID());
        const std::vector<UUID> children = EntityIds(loadedOrdered.GetChildren(loadedParent));
        HK_CHECK_EQ(children.size(), static_cast<std::size_t>(2));
        HK_CHECK(children[0] == childB.GetUUID());
        HK_CHECK(children[1] == childA.GetUUID());
    }

    // Legacy scenes without RootOrder still load and expose valid roots.
    {
        const std::filesystem::path legacyPath = Hockey::Paths::TempFile("legacy_no_root_order.scene.yaml");
        const std::string yaml = R"(Scene:
  Name: Legacy
  Mode: Edit
  Version: 1
Entities:
  - Entity: 1001
    NameComponent: {Name: RootA}
    ObjectSettingsComponent: {Tag: Untagged, Layer: Default, Static: false}
    ActiveComponent: {Active: true}
    TransformComponent: {Position: [0, 0, 0], Rotation: [0, 0, 0], Scale: [1, 1, 1]}
    ChildrenComponent: {Children: []}
  - Entity: 1002
    NameComponent: {Name: RootB}
    ObjectSettingsComponent: {Tag: Untagged, Layer: Default, Static: false}
    ActiveComponent: {Active: true}
    TransformComponent: {Position: [0, 0, 0], Rotation: [0, 0, 0], Scale: [1, 1, 1]}
    ChildrenComponent: {Children: []}
)";
        HK_CHECK(static_cast<bool>(FileSystem::WriteTextFile(legacyPath, yaml)));

        Scene legacy("Empty");
        HK_CHECK(static_cast<bool>(SceneSerializer(legacy).Deserialize(legacyPath)));

        const std::vector<UUID> roots = EntityIds(legacy.GetRootEntities());
        HK_CHECK_EQ(roots.size(), static_cast<std::size_t>(2));
        HK_CHECK(roots[0] == UUID(1001));
        HK_CHECK(roots[1] == UUID(1002));
    }

    {
        Scene source("StickAttachmentRoundTrip");
        Entity player = source.CreateEntity("Player With Stick Attachment");
        Entity stick = source.CreateEntity("Stick Child");
        source.SetParent(stick, player, false);

        StickAttachmentComponent attachment;
        attachment.stickEntityId = stick.GetUUID();
        player.AddComponent<StickAttachmentComponent>(attachment);

        Entity duplicate = source.DuplicateEntity(player);
        HK_CHECK(duplicate.HasComponent<StickAttachmentComponent>());
        if (duplicate.HasComponent<StickAttachmentComponent>()) {
            const std::vector<Entity> duplicateChildren = source.GetChildren(duplicate);
            HK_CHECK_EQ(duplicateChildren.size(), static_cast<std::size_t>(1));
            if (!duplicateChildren.empty()) {
                HK_CHECK_EQ(duplicate.GetComponent<StickAttachmentComponent>().stickEntityId,
                            duplicateChildren.front().GetUUID());
                HK_CHECK(duplicate.GetComponent<StickAttachmentComponent>().stickEntityId != stick.GetUUID());
            }
        }

        const std::filesystem::path path = Paths::TempFile("stick_attachment_scene.scene.yaml");
        HK_CHECK(static_cast<bool>(SceneSerializer(source).Serialize(path)));

        Scene loaded("LoadedStickAttachmentRoundTrip");
        HK_CHECK(static_cast<bool>(SceneSerializer(loaded).Deserialize(path)));
        Entity loadedPlayer = loaded.FindEntityByName("Player With Stick Attachment");
        HK_CHECK(loadedPlayer.IsValid());
        HK_CHECK(loadedPlayer.HasComponent<StickAttachmentComponent>());
        if (loadedPlayer.IsValid() && loadedPlayer.HasComponent<StickAttachmentComponent>()) {
            const std::vector<Entity> loadedChildren = loaded.GetChildren(loadedPlayer);
            HK_CHECK_EQ(loadedChildren.size(), static_cast<std::size_t>(1));
            if (!loadedChildren.empty()) {
                HK_CHECK_EQ(loadedPlayer.GetComponent<StickAttachmentComponent>().stickEntityId,
                            loadedChildren.front().GetUUID());
            }
        }
    }
}
