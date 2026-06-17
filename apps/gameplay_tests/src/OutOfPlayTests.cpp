#include "Test.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Rink/OutOfPlaySystem.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

using namespace Hockey;

namespace {

Entity AddMarker(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

void AddSpawn(Scene& scene, Team team, PlayerRole role, int index, const glm::vec3& position) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.role = role;
    component.index = index;
    spawn.AddComponent<SpawnPointComponent>(component);
}

void BuildValidGameplayScene(Scene& scene) {
    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>();
    rink.AddComponent<PlayAreaComponent>().halfExtents = {10.0f, 5.0f, 10.0f};
    rink.AddComponent<OutOfPlayComponent>().minY = -2.0f;

    AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f}).AddComponent<PuckComponent>();
    AddMarker(scene, "Home Goal", {0.0f, 0.0f, -8.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Home;
    AddMarker(scene, "Away Goal", {0.0f, 0.0f, 8.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Away;
    AddMarker(scene, "Center Faceoff", {0.0f, 0.0f, 0.0f}).AddComponent<FaceoffSpotComponent>().index = 0;

    for (int i = 0; i < 3; ++i) {
        AddSpawn(scene, Team::Home, PlayerRole::Skater, i, {-3.0f + static_cast<float>(i), 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, PlayerRole::Skater, i, {-3.0f + static_cast<float>(i), 0.0f, 5.0f});
    }
    AddSpawn(scene, Team::Home, PlayerRole::Goalie, 0, {0.0f, 0.0f, -7.0f});
    AddSpawn(scene, Team::Away, PlayerRole::Goalie, 0, {0.0f, 0.0f, 7.0f});
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

bool SawEvent(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunOutOfPlayTests() {
    HockeyTest::BeginSuite("OutOfPlayTests");

    Scene scene("OutOfPlayScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.postGoalDelaySeconds = 10.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

    Entity puck = FindPuck(scene);
    HK_CHECK(puck.IsValid());
    puck.GetComponent<TransformComponent>().localPosition = {0.0f, -3.0f, 0.0f};
    HK_CHECK(OutOfPlaySystem::IsPuckOutOfPlay(scene, puck, settings));

    puck.GetComponent<TransformComponent>().localPosition = {12.0f, 0.0f, 0.0f};
    HK_CHECK(OutOfPlaySystem::IsPuckOutOfPlay(scene, puck, settings));

    settings.allowOutOfPlay = false;
    HK_CHECK(!OutOfPlaySystem::IsPuckOutOfPlay(scene, puck, settings));
    settings.allowOutOfPlay = true;

    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    std::vector<GameplayEvent> events = world.DrainEvents();
    HK_CHECK(SawEvent(events, GameplayEventType::PuckOutOfPlay));
    HK_CHECK(SawEvent(events, GameplayEventType::ResetStarted));
    HK_CHECK_EQ(FindMatch(scene).GetComponent<MatchStateComponent>().phase, MatchPhase::GoalScored);
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Resetting);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().inPlay);
}
