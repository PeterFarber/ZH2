#include "Test.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/ScoreSystem.hpp"
#include "Hockey/Gameplay/Rink/GoalDetection.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

using namespace Hockey;

namespace {

Entity AddMarker(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

void AddSpawn(Scene& scene, Team team, const glm::vec3& position, bool faceoffSpawn = false) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.faceoffSpawn = faceoffSpawn;
    spawn.AddComponent<SpawnPointComponent>(component);
}

void BuildValidGameplayScene(Scene& scene) {
    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>();
    rink.AddComponent<PlayAreaComponent>();

    AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f}).AddComponent<PuckComponent>();
    AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Home;
    AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Away;
    for (int i = 0; i < 4; ++i) {
        AddSpawn(scene, Team::Home, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, 5.0f});
    }
    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, true);
    }
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

Entity FindGoal(Scene& scene, GameplayTeam defendingTeam) {
    auto view = scene.Registry().view<GoalGameplayComponent>();
    for (const entt::entity handle : view) {
        Entity goal(handle, &scene);
        if (goal.GetComponent<GoalGameplayComponent>().defendingTeam == defendingTeam) {
            return goal;
        }
    }
    return {};
}

Entity FindTeamState(Scene& scene, GameplayTeam team) {
    auto view = scene.Registry().view<TeamStateComponent>();
    for (const entt::entity handle : view) {
        Entity entity(handle, &scene);
        if (entity.GetComponent<TeamStateComponent>().team == team) {
            return entity;
        }
    }
    return {};
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

void RunGoalDetectionTests() {
    HockeyTest::BeginSuite("GoalDetectionTests");

    Scene scene("GoalDetectionScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.postGoalDelaySeconds = 10.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();

    MatchStateComponent& match = FindMatch(scene).GetComponent<MatchStateComponent>();
    match.phase = MatchPhase::Playing;
    match.clockRunning = true;

    Entity puck = FindPuck(scene);
    Entity homeGoal = FindGoal(scene, GameplayTeam::Home);
    Entity awayGoal = FindGoal(scene, GameplayTeam::Away);
    HK_CHECK(puck.IsValid());
    HK_CHECK(homeGoal.IsValid());
    HK_CHECK(awayGoal.IsValid());

    GameplayEventQueue events;
    HK_CHECK(GoalDetection::HandleGoalTrigger(scene, homeGoal, puck, settings, events));
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Away), 1u);
    HK_CHECK(SawEvent(events.Drain(), GameplayEventType::GoalScored));

    Entity playerBody = scene.FindEntityByName("HomeSkater0");
    if (!playerBody.IsValid()) {
        auto view = scene.Registry().view<PlayerComponent>();
        const auto it = view.begin();
        playerBody = it == view.end() ? Entity{} : Entity(*it, &scene);
    }
    HK_CHECK(playerBody.IsValid());
    match.phase = MatchPhase::Playing;
    HK_CHECK(!GoalDetection::HandleGoalTrigger(scene, homeGoal, playerBody, settings, events));
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Away), 1u);

    match.phase = MatchPhase::Playing;
    match.homeScore = 0;
    match.awayScore = 0;
    FindTeamState(scene, GameplayTeam::Away).GetComponent<TeamStateComponent>().score = 0;
    puck.GetComponent<TransformComponent>().localPosition = awayGoal.GetComponent<TransformComponent>().localPosition;
    GoalDetection::FixedUpdate(scene, settings, events);
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Home), 1u);
}

void RunScoreTests() {
    HockeyTest::BeginSuite("ScoreTests");

    Scene scene("ScoreScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.postGoalDelaySeconds = 10.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

    Entity puck = FindPuck(scene);
    Entity homeGoal = FindGoal(scene, GameplayTeam::Home);
    GameplayEventQueue events;
    HK_CHECK(GoalDetection::HandleGoalTrigger(scene, homeGoal, puck, settings, events));

    std::vector<GameplayEvent> goalEvents = events.Drain();
    Entity match = FindMatch(scene);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().awayScore, 1u);
    HK_CHECK_EQ(match.GetComponent<ScoreComponent>().awayScore, 1u);
    HK_CHECK_EQ(FindTeamState(scene, GameplayTeam::Away).GetComponent<TeamStateComponent>().score, 1u);
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Away), 1u);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::GoalScored);
    HK_CHECK(!match.GetComponent<MatchStateComponent>().clockRunning);
    HK_CHECK(SawEvent(goalEvents, GameplayEventType::GoalScored));
    HK_CHECK(SawEvent(goalEvents, GameplayEventType::ScoreChanged));
    HK_CHECK(SawEvent(goalEvents, GameplayEventType::ResetStarted));

    HK_CHECK(!GoalDetection::HandleGoalTrigger(scene, homeGoal, puck, settings, events));
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Away), 1u);

    match.GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
    Entity awayGoal = FindGoal(scene, GameplayTeam::Away);
    ScoreSystem::AddGoal(scene, GameplayTeam::Home, settings, events);
    HK_CHECK_EQ(ScoreSystem::GetScore(scene, GameplayTeam::Home), 1u);
    HK_CHECK_EQ(FindTeamState(scene, GameplayTeam::Home).GetComponent<TeamStateComponent>().score, 1u);
    HK_CHECK(awayGoal.IsValid());
}
