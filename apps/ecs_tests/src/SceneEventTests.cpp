#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

namespace {

bool HasEventType(const Scene& scene, SceneEventType type) {
    for (const auto& event : scene.GetPendingEvents()) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunSceneEventTests() {
    HockeyTest::BeginSuite("SceneEventTests");

    Scene scene("Test");
    scene.ClearPendingEvents();

    Entity e = scene.CreateEntity("E");
    HK_CHECK(HasEventType(scene, SceneEventType::EntityCreated));

    e.AddComponent<TeamComponent>();
    HK_CHECK(HasEventType(scene, SceneEventType::ComponentAdded));

    e.RemoveComponent<TeamComponent>();
    HK_CHECK(HasEventType(scene, SceneEventType::ComponentRemoved));

    Entity parent = scene.CreateEntity("Parent");
    scene.SetParent(e, parent);
    HK_CHECK(HasEventType(scene, SceneEventType::ParentChanged));

    e.SetName("Renamed");
    HK_CHECK(HasEventType(scene, SceneEventType::EntityRenamed));

    e.SetActive(false);
    HK_CHECK(HasEventType(scene, SceneEventType::ActiveChanged));

    Entity doomed = scene.CreateEntity("Doomed");
    scene.DestroyEntity(doomed);
    HK_CHECK(HasEventType(scene, SceneEventType::EntityDestroyed));

    scene.ClearPendingEvents();
    HK_CHECK(scene.GetPendingEvents().empty());
}
