#include "Test.hpp"

#include <entt/entt.hpp>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
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

void AddSpawn(Scene& scene,
              Team team,
              PlayerRole role,
              int index,
              const glm::vec3& position,
              const std::filesystem::path& prefabPath = {}) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.role = role;
    component.index = index;
    component.playerPrefabPath = prefabPath;
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

Entity FindPlayer(Scene& scene, PlayerSlot slot) {
    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        if (player.GetComponent<PlayerComponent>().slot == slot) {
            return player;
        }
    }
    return {};
}

Entity FindSpawn(Scene& scene, Team team, PlayerRole role, int index) {
    auto view = scene.Registry().view<SpawnPointComponent>();
    for (const entt::entity handle : view) {
        Entity spawn(handle, &scene);
        const SpawnPointComponent& component = spawn.GetComponent<SpawnPointComponent>();
        if (component.team == team && component.role == role && component.index == index) {
            return spawn;
        }
    }
    return {};
}

std::filesystem::path SavePlayerPrefab() {
    Scene prefabScene("PlayerPrefab");
    Entity root = prefabScene.CreateEntity("Authored Player Prefab");
    root.AddComponent<CameraRigMarkerComponent>().purpose = "PrefabSpawnMarker";
    const std::filesystem::path path = Paths::TempFile("spawn_player.prefab.yaml");
    HK_CHECK(static_cast<bool>(PrefabSerializer::Save(prefabScene, root, path)));
    return path;
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

    {
        Scene prefabSpawnScene("PrefabSpawnScene");
        BuildValidGameplayScene(prefabSpawnScene);
        Entity homeSpawn = FindSpawn(prefabSpawnScene, Team::Home, PlayerRole::Skater, 0);
        HK_CHECK(homeSpawn.IsValid() && homeSpawn.HasComponent<SpawnPointComponent>());
        const std::filesystem::path prefabPath = SavePlayerPrefab();
        if (homeSpawn.IsValid() && homeSpawn.HasComponent<SpawnPointComponent>()) {
            homeSpawn.GetComponent<SpawnPointComponent>().playerPrefabPath = prefabPath;
        }

        HK_CHECK(static_cast<bool>(MatchSystem::InitializeMatch(prefabSpawnScene, settings)));
        Entity prefabPlayer = FindPlayer(prefabSpawnScene, PlayerSlot::HomeSkater0);
        HK_CHECK(prefabPlayer.IsValid());
        if (prefabPlayer.IsValid()) {
            HK_CHECK(prefabPlayer.HasComponent<PrefabComponent>());
            HK_CHECK(prefabPlayer.HasComponent<CameraRigMarkerComponent>());
            if (prefabPlayer.HasComponent<CameraRigMarkerComponent>()) {
                HK_CHECK_EQ(prefabPlayer.GetComponent<CameraRigMarkerComponent>().purpose,
                            std::string("PrefabSpawnMarker"));
            }
            HK_CHECK_EQ(prefabPlayer.GetComponent<NameComponent>().name, std::string("HomeSkater0"));
            HK_CHECK_NEAR(prefabPlayer.GetComponent<TransformComponent>().localPosition.z, -4.0f, 0.001f);
            HK_CHECK(prefabPlayer.HasComponent<PlayerRuntimeComponent>());
            HK_CHECK(prefabPlayer.HasComponent<SkaterComponent>());
            HK_CHECK(prefabPlayer.HasComponent<StickComponent>());
        }
    }
}
