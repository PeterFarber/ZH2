#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

namespace {

bool HasEvent(const Scene& scene, SceneEventType type, const char* componentName) {
    for (const auto& event : scene.GetPendingEvents()) {
        if (event.type == type && event.componentName == componentName) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunComponentTests() {
    HockeyTest::BeginSuite("ComponentTests");

    Scene scene("Test");
    Entity e = scene.CreateEntity("Entity");

    // AddComponent + GetComponent + HasComponent.
    HK_CHECK(!e.HasComponent<TeamComponent>());
    TeamComponent& team = e.AddComponent<TeamComponent>(TeamComponent{Team::Away});
    HK_CHECK(e.HasComponent<TeamComponent>());
    HK_CHECK(team.team == Team::Away);
    HK_CHECK(e.GetComponent<TeamComponent>().team == Team::Away);

    // AddOrReplaceComponent overwrites.
    e.AddOrReplaceComponent<TeamComponent>(TeamComponent{Team::Home});
    HK_CHECK(e.GetComponent<TeamComponent>().team == Team::Home);

    // HasAll / HasAny.
    HK_CHECK((e.HasAll<TransformComponent, NameComponent>()));
    HK_CHECK((e.HasAny<TeamComponent, PuckComponent>()));
    HK_CHECK((!e.HasAll<TeamComponent, PuckComponent>()));

    // Component add event fired for TeamComponent.
    HK_CHECK(HasEvent(scene, SceneEventType::ComponentAdded, "TeamComponent"));

    // RemoveComponent works and fires event.
    scene.ClearPendingEvents();
    e.RemoveComponent<TeamComponent>();
    HK_CHECK(!e.HasComponent<TeamComponent>());
    HK_CHECK(HasEvent(scene, SceneEventType::ComponentRemoved, "TeamComponent"));

    // Protected components cannot be removed.
    e.RemoveComponent<IDComponent>();
    HK_CHECK(e.HasComponent<IDComponent>());
    e.RemoveComponent<TransformComponent>();
    HK_CHECK(e.HasComponent<TransformComponent>());
    e.RemoveComponent<ChildrenComponent>();
    HK_CHECK(e.HasComponent<ChildrenComponent>());
}
