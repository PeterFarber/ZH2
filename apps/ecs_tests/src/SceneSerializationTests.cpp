#include "Test.hpp"

#include <filesystem>

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

} // namespace

void RunSceneSerializationTests() {
    HockeyTest::BeginSuite("SceneSerializationTests");

    Scene scene("Main Rink");

    Entity rink = scene.CreateEntity("Rink");
    rink.AddComponent<RinkComponent>(RinkComponent{"Test Rink"});
    rink.GetComponent<TransformComponent>().localPosition = glm::vec3(1.0f, 2.0f, 3.0f);

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
    HK_CHECK_EQ(loaded.EntityCount(), static_cast<std::size_t>(4));

    Entity lr = loaded.FindEntityByUUID(rinkId);
    HK_CHECK(lr.IsValid());
    HK_CHECK_EQ(lr.GetName(), std::string("Rink"));
    HK_CHECK(lr.HasComponent<RinkComponent>());
    HK_CHECK_EQ(lr.GetComponent<RinkComponent>().rinkName, std::string("Test Rink"));
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
}
