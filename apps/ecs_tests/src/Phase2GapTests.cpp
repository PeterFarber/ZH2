#include "Test.hpp"

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"

using namespace Hockey;

void RunActiveHierarchyTests() {
    HockeyTest::BeginSuite("ActiveHierarchyTests");

    Scene scene("ActiveHierarchy");
    Entity root = scene.CreateEntity("Root");
    Entity child = scene.CreateEntity("Child");
    Entity grandchild = scene.CreateEntity("Grandchild");
    scene.SetParent(child, root, false);
    scene.SetParent(grandchild, child, false);

    // Everything starts active locally and in hierarchy.
    HK_CHECK(root.IsActiveInHierarchy());
    HK_CHECK(child.IsActiveInHierarchy());
    HK_CHECK(grandchild.IsActiveInHierarchy());

    // Disabling the root propagates down: descendants stay locally active but
    // are inactive in the hierarchy.
    scene.SetActive(root, false);
    HK_CHECK(!root.IsActiveInHierarchy());
    HK_CHECK(child.IsActive());
    HK_CHECK(!child.IsActiveInHierarchy());
    HK_CHECK(!grandchild.IsActiveInHierarchy());

    // Re-enabling the root restores the subtree.
    scene.SetActive(root, true);
    HK_CHECK(root.IsActiveInHierarchy());
    HK_CHECK(child.IsActiveInHierarchy());
    HK_CHECK(grandchild.IsActiveInHierarchy());

    // Disabling a middle node only affects it and below.
    scene.SetActive(child, false);
    HK_CHECK(root.IsActiveInHierarchy());
    HK_CHECK(!child.IsActiveInHierarchy());
    HK_CHECK(!grandchild.IsActiveInHierarchy());
}

void RunSceneClearTests() {
    HockeyTest::BeginSuite("SceneClearTests");

    Scene scene("Clearable");
    scene.CreateEntity("A");
    Entity b = scene.CreateEntity("B");
    scene.CreateEntity("C");
    const UUID bId = b.GetUUID();
    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(3));

    scene.Clear();
    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(0));
    HK_CHECK(!scene.ContainsUUID(bId));
    HK_CHECK(!scene.FindEntityByUUID(bId).IsValid());

    // Scene is reusable after Clear.
    Entity fresh = scene.CreateEntity("Fresh");
    HK_CHECK(fresh.IsValid());
    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(1));
}

void RunMarkerSerializationTests() {
    HockeyTest::BeginSuite("MarkerSerializationTests");

    Scene scene("Markers");

    Entity spawn = scene.CreateEntity("Spawn");
    spawn.AddComponent<SpawnPointComponent>(SpawnPointComponent{Team::Away, PlayerRole::Goalie, 2});
    spawn.AddComponent<TeamComponent>(TeamComponent{Team::Away});
    spawn.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{PlayerRole::Goalie});

    Entity faceoff = scene.CreateEntity("Faceoff");
    faceoff.AddComponent<FaceoffSpotComponent>(FaceoffSpotComponent{5});

    Entity area = scene.CreateEntity("Area");
    area.AddComponent<PlayAreaComponent>(PlayAreaComponent{glm::vec3(11.0f, 22.0f, 33.0f)});

    Entity rig = scene.CreateEntity("Rig");
    rig.AddComponent<CameraRigMarkerComponent>(CameraRigMarkerComponent{"BenchCam"});

    const UUID spawnId = spawn.GetUUID();
    const UUID faceoffId = faceoff.GetUUID();
    const UUID areaId = area.GetUUID();
    const UUID rigId = rig.GetUUID();

    const std::filesystem::path path = Paths::TempFile("markers_roundtrip.scene.yaml");
    SceneSerializer serializer(scene);
    HK_CHECK(static_cast<bool>(serializer.Serialize(path)));

    Scene loaded("Empty");
    SceneSerializer deserializer(loaded);
    HK_CHECK(static_cast<bool>(deserializer.Deserialize(path)));

    Entity ls = loaded.FindEntityByUUID(spawnId);
    HK_CHECK(ls.IsValid() && ls.HasComponent<SpawnPointComponent>());
    if (ls.IsValid() && ls.HasComponent<SpawnPointComponent>()) {
        const auto& sp = ls.GetComponent<SpawnPointComponent>();
        HK_CHECK(sp.team == Team::Away);
        HK_CHECK(sp.role == PlayerRole::Goalie);
        HK_CHECK_EQ(sp.index, 2);
    }
    HK_CHECK(ls.HasComponent<TeamComponent>() && ls.GetComponent<TeamComponent>().team == Team::Away);
    HK_CHECK(ls.HasComponent<PlayerRoleComponent>() &&
             ls.GetComponent<PlayerRoleComponent>().role == PlayerRole::Goalie);

    Entity lf = loaded.FindEntityByUUID(faceoffId);
    HK_CHECK(lf.IsValid() && lf.HasComponent<FaceoffSpotComponent>());
    if (lf.IsValid() && lf.HasComponent<FaceoffSpotComponent>()) {
        HK_CHECK_EQ(lf.GetComponent<FaceoffSpotComponent>().index, 5);
    }

    Entity la = loaded.FindEntityByUUID(areaId);
    HK_CHECK(la.IsValid() && la.HasComponent<PlayAreaComponent>());
    if (la.IsValid() && la.HasComponent<PlayAreaComponent>()) {
        HK_CHECK_NEAR(la.GetComponent<PlayAreaComponent>().halfExtents.y, 22.0f, 1e-4);
    }

    Entity lr = loaded.FindEntityByUUID(rigId);
    HK_CHECK(lr.IsValid() && lr.HasComponent<CameraRigMarkerComponent>());
    if (lr.IsValid() && lr.HasComponent<CameraRigMarkerComponent>()) {
        HK_CHECK_EQ(lr.GetComponent<CameraRigMarkerComponent>().purpose, std::string("BenchCam"));
    }
}

void RunPrefabOverridePersistenceTests() {
    HockeyTest::BeginSuite("PrefabOverridePersistenceTests");

    Scene scene("Overrides");
    Entity puck = scene.CreateEntity("Puck");
    puck.AddComponent<PuckComponent>(PuckComponent{true});
    Entity rink = scene.CreateEntity("Rink");
    rink.AddComponent<RinkComponent>(RinkComponent{"Main"});

    const UUID puckId = puck.GetUUID();
    const UUID rinkId = rink.GetUUID();

    PrefabOverride nameOverride;
    nameOverride.entityId = rinkId;
    nameOverride.componentName = "NameComponent";
    nameOverride.fieldName = "Name";
    nameOverride.value = YAML::Node("Overridden Rink");
    scene.PrefabOverrides().AddOverride(nameOverride);

    PrefabOverride puckOverride;
    puckOverride.entityId = puckId;
    puckOverride.componentName = "PuckComponent";
    puckOverride.fieldName = "StartsInPlay";
    puckOverride.value = YAML::Node(false);
    scene.PrefabOverrides().AddOverride(puckOverride);

    HK_CHECK_EQ(scene.PrefabOverrides().Overrides().size(), static_cast<std::size_t>(2));

    const std::filesystem::path path = Paths::TempFile("override_roundtrip.scene.yaml");
    SceneSerializer serializer(scene);
    HK_CHECK(static_cast<bool>(serializer.Serialize(path)));

    Scene loaded("Empty");
    SceneSerializer deserializer(loaded);
    HK_CHECK(static_cast<bool>(deserializer.Deserialize(path)));

    const auto& overrides = loaded.PrefabOverrides().Overrides();
    HK_CHECK_EQ(overrides.size(), static_cast<std::size_t>(2));

    bool foundName = false;
    bool foundPuck = false;
    for (const PrefabOverride& ov : overrides) {
        if (ov.componentName == "NameComponent" && ov.fieldName == "Name") {
            foundName = ov.entityId == rinkId && ov.value.as<std::string>() == "Overridden Rink";
        } else if (ov.componentName == "PuckComponent" && ov.fieldName == "StartsInPlay") {
            foundPuck = ov.entityId == puckId && ov.value.as<bool>() == false;
        }
    }
    HK_CHECK_MSG(foundName, "Name override survives scene round-trip");
    HK_CHECK_MSG(foundPuck, "Puck override survives scene round-trip");

    // A scene without an override section loads with an empty set.
    Scene plain("Plain");
    plain.CreateEntity("Solo");
    const std::filesystem::path plainPath = Paths::TempFile("plain_roundtrip.scene.yaml");
    SceneSerializer plainSerializer(plain);
    HK_CHECK(static_cast<bool>(plainSerializer.Serialize(plainPath)));
    Scene plainLoaded("Empty");
    SceneSerializer plainDeserializer(plainLoaded);
    HK_CHECK(static_cast<bool>(plainDeserializer.Deserialize(plainPath)));
    HK_CHECK(plainLoaded.PrefabOverrides().Empty());
}
