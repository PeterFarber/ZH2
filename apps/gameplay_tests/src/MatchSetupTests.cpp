#include "Test.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/MatchSystem.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

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
    rink.AddComponent<RinkComponent>().rinkName = "Test Rink";
    rink.AddComponent<PlayAreaComponent>();

    Entity puck = AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f});
    puck.AddComponent<PuckComponent>();

    Entity homeGoal = AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f});
    homeGoal.AddComponent<GoalComponent>().defendingTeam = Team::Home;
    Entity awayGoal = AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f});
    awayGoal.AddComponent<GoalComponent>().defendingTeam = Team::Away;

    Entity faceoff = AddMarker(scene, "Center Faceoff", {0.0f, 0.0f, 0.0f});
    faceoff.AddComponent<FaceoffSpotComponent>().index = 0;

    for (int i = 0; i < 3; ++i) {
        AddSpawn(scene, Team::Home, PlayerRole::Skater, i, {-4.0f + static_cast<float>(i) * 2.0f, 0.0f, -4.0f});
        AddSpawn(scene, Team::Away, PlayerRole::Skater, i, {-4.0f + static_cast<float>(i) * 2.0f, 0.0f, 4.0f});
    }
    AddSpawn(scene, Team::Home, PlayerRole::Goalie, 0, {0.0f, 0.0f, -24.0f});
    AddSpawn(scene, Team::Away, PlayerRole::Goalie, 0, {0.0f, 0.0f, 24.0f});
}

} // namespace

void RunMatchSetupTests() {
    HockeyTest::BeginSuite("MatchSetupTests");

    RegisterGameplayComponents();

    {
        Scene missing("MissingGameplayData");
        const auto issues = SceneValidator::Validate(missing);
        HK_CHECK(SceneValidator::HasErrors(issues));
        HK_CHECK(!MatchSystem::InitializeMatch(missing));
    }

    Scene scene("ValidGameplayData");
    BuildValidGameplayScene(scene);
    const auto issues = SceneValidator::Validate(scene);
    HK_CHECK_MSG(!SceneValidator::HasErrors(issues), "valid gameplay scene has no validation errors");

    GameplaySettings settings;
    settings.periodLengthSeconds = 180.0f;
    HK_CHECK_MSG(static_cast<bool>(MatchSystem::InitializeMatch(scene, settings)), "match initializes");

    Entity match = scene.FindEntityByName("Match State");
    HK_CHECK(match.IsValid());
    HK_CHECK(match.HasComponent<MatchStateComponent>());
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::FaceoffSetup);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().periodCount, settings.periodCount);
    HK_CHECK_NEAR(match.GetComponent<MatchStateComponent>().periodTimeRemaining, 180.0f, 0.0001f);
    HK_CHECK(match.GetComponent<MatchStateComponent>().matchInitialized);

    int homeSkaters = 0;
    int awaySkaters = 0;
    int homeGoalies = 0;
    int awayGoalies = 0;
    auto playerView = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : playerView) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        if (playerComponent.team == GameplayTeam::Home && playerComponent.role == GameplayRole::Skater) ++homeSkaters;
        if (playerComponent.team == GameplayTeam::Away && playerComponent.role == GameplayRole::Skater) ++awaySkaters;
        if (playerComponent.team == GameplayTeam::Home && playerComponent.role == GameplayRole::Goalie) ++homeGoalies;
        if (playerComponent.team == GameplayTeam::Away && playerComponent.role == GameplayRole::Goalie) ++awayGoalies;

        HK_CHECK(player.HasComponent<PlayerRuntimeComponent>());
        HK_CHECK(player.HasComponent<StickComponent>());
        HK_CHECK(player.HasComponent<ShotComponent>());
        HK_CHECK(player.HasComponent<PassComponent>());
        HK_CHECK(player.HasComponent<CheckComponent>());
    }
    HK_CHECK_EQ(homeSkaters, 3);
    HK_CHECK_EQ(awaySkaters, 3);
    HK_CHECK_EQ(homeGoalies, 1);
    HK_CHECK_EQ(awayGoalies, 1);

    Entity homeTeam = scene.FindEntityByName("Home Team State");
    Entity awayTeam = scene.FindEntityByName("Away Team State");
    HK_CHECK(homeTeam.HasComponent<TeamStateComponent>());
    HK_CHECK(awayTeam.HasComponent<TeamStateComponent>());
    HK_CHECK_EQ(homeTeam.GetComponent<TeamStateComponent>().players.size(), static_cast<std::size_t>(4));
    HK_CHECK_EQ(awayTeam.GetComponent<TeamStateComponent>().players.size(), static_cast<std::size_t>(4));
    HK_CHECK(homeTeam.GetComponent<TeamStateComponent>().goalie.IsValid());
    HK_CHECK(awayTeam.GetComponent<TeamStateComponent>().goalie.IsValid());

    Entity puck = scene.FindEntityByName("Puck");
    HK_CHECK(puck.HasComponent<PuckGameplayComponent>());
    HK_CHECK(puck.HasComponent<PuckRuntimeComponent>());

    Entity homeGoal = scene.FindEntityByName("Home Goal");
    Entity awayGoal = scene.FindEntityByName("Away Goal");
    HK_CHECK_EQ(homeGoal.GetComponent<GoalGameplayComponent>().defendingTeam, GameplayTeam::Home);
    HK_CHECK_EQ(homeGoal.GetComponent<GoalGameplayComponent>().scoringTeam, GameplayTeam::Away);
    HK_CHECK_EQ(awayGoal.GetComponent<GoalGameplayComponent>().defendingTeam, GameplayTeam::Away);
    HK_CHECK_EQ(awayGoal.GetComponent<GoalGameplayComponent>().scoringTeam, GameplayTeam::Home);
}
