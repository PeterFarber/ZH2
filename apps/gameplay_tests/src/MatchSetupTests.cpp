#include "Test.hpp"

#include <cmath>
#include <filesystem>
#include <vector>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/MatchSystem.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

using namespace Hockey;

namespace {

Entity AddMarker(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

Entity AddSpawn(Scene& scene,
                Team team,
                const glm::vec3& position,
                const std::filesystem::path& prefabPath = {},
                bool faceoffSpawn = false) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.faceoffSpawn = faceoffSpawn;
    component.playerPrefabPath = prefabPath;
    spawn.AddComponent<SpawnPointComponent>(component);
    return spawn;
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

    for (int i = 0; i < 4; ++i) {
        AddSpawn(scene, Team::Home, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, 5.0f});
    }
    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, {}, true);
    }
}

void BuildRoleMarkedGameplayScene(Scene& scene, glm::vec3& outHomeGoalieSpawn, glm::vec3& outAwayGoalieSpawn) {
    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>().rinkName = "Role Marked Rink";
    rink.AddComponent<PlayAreaComponent>();

    Entity puck = AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f});
    puck.AddComponent<PuckComponent>();

    Entity homeGoal = AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f});
    homeGoal.AddComponent<GoalComponent>().defendingTeam = Team::Home;
    Entity awayGoal = AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f});
    awayGoal.AddComponent<GoalComponent>().defendingTeam = Team::Away;

    outHomeGoalieSpawn = {11.0f, 0.0f, -9.0f};
    outAwayGoalieSpawn = {-13.0f, 0.0f, 9.0f};

    const glm::vec3 homePositions[] = {
        {-17.0f, 0.0f, -9.0f},
        outHomeGoalieSpawn,
        {-5.0f, 0.0f, -9.0f},
        {1.0f, 0.0f, -9.0f},
    };
    const glm::vec3 awayPositions[] = {
        {-19.0f, 0.0f, 9.0f},
        {-7.0f, 0.0f, 9.0f},
        outAwayGoalieSpawn,
        {5.0f, 0.0f, 9.0f},
    };

    for (int i = 0; i < 4; ++i) {
        Entity home = AddSpawn(scene, Team::Home, homePositions[i]);
        home.AddComponent<PlayerRoleComponent>(
            PlayerRoleComponent{i == 1 ? PlayerRole::Goalie : PlayerRole::Skater});

        Entity away = AddSpawn(scene, Team::Away, awayPositions[i]);
        away.AddComponent<PlayerRoleComponent>(
            PlayerRoleComponent{i == 2 ? PlayerRole::Goalie : PlayerRole::Skater});
    }

    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, {}, true);
    }
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

Entity FindSpawn(Scene& scene, Team team, bool faceoffSpawn, std::size_t skip = 0) {
    auto view = scene.Registry().view<SpawnPointComponent>();
    for (const entt::entity handle : view) {
        Entity spawn(handle, &scene);
        const SpawnPointComponent& component = spawn.GetComponent<SpawnPointComponent>();
        if (component.team == team && component.faceoffSpawn == faceoffSpawn) {
            if (skip > 0) {
                --skip;
                continue;
            }
            return spawn;
        }
    }
    return {};
}

glm::vec3 ExpectedFacingToward(const glm::vec3& from, const glm::vec3& target) {
    glm::vec3 direction = target - from;
    direction.y = 0.0f;
    return glm::length(direction) > 0.0001f ? glm::normalize(direction) : glm::vec3{0.0f, 0.0f, 1.0f};
}

float ExpectedYawDegrees(const glm::vec3& facing) {
    constexpr float kRadiansToDegrees = 57.29577951308232f;
    return std::atan2(facing.x, facing.z) * kRadiansToDegrees;
}

void CheckPlayerFacesPuck(Entity player, Entity puck) {
    const TransformComponent& playerTransform = player.GetComponent<TransformComponent>();
    const glm::vec3 expectedFacing =
        ExpectedFacingToward(playerTransform.localPosition, puck.GetComponent<TransformComponent>().localPosition);
    const PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();

    HK_CHECK_NEAR(runtime.facingDirection.x, expectedFacing.x, 0.001f);
    HK_CHECK_NEAR(runtime.facingDirection.y, 0.0f, 0.001f);
    HK_CHECK_NEAR(runtime.facingDirection.z, expectedFacing.z, 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.x, 0.0f, 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.y, ExpectedYawDegrees(expectedFacing), 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.z, 0.0f, 0.001f);
}

void CheckAllPlayersFacePuck(Scene& scene) {
    Entity puck = scene.FindEntityByName("Puck");
    HK_CHECK(puck.IsValid());
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();
    int checked = 0;
    for (const entt::entity handle : view) {
        CheckPlayerFacesPuck(Entity(handle, &scene), puck);
        ++checked;
    }
    HK_CHECK_EQ(checked, 8);
}

std::filesystem::path SavePlayerPrefab() {
    Scene prefabScene("PlayerPrefab");
    Entity root = prefabScene.CreateEntity("Authored Player Prefab");
    root.AddComponent<CameraRigMarkerComponent>().purpose = "PrefabSpawnMarker";

    Entity stick = prefabScene.CreateEntity("Stale Embedded Stick Child");
    stick.GetComponent<TransformComponent>().localPosition = {0.0f, 0.75f, 0.95f};

    StickComponent stickComponent;
    stickComponent.reach = 99.0f;
    stickComponent.width = 99.0f;
    stickComponent.localOffset = {0.0f, 0.1f, 1.25f};
    stick.AddComponent<StickComponent>(stickComponent);
    prefabScene.SetParent(stick, root, false);

    const std::filesystem::path path = Paths::TempFile("spawn_player.prefab.yaml");
    HK_CHECK(static_cast<bool>(PrefabSerializer::Save(prefabScene, root, path)));
    return path;
}

std::filesystem::path SaveStickPrefab(const char* fileName, const char* entityName, float staleReach) {
    Scene prefabScene(entityName);
    Entity root = prefabScene.CreateEntity(entityName);
    root.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.1f};
    root.GetComponent<TransformComponent>().localScale = {0.67f, 0.67f, 0.67f};

    StickComponent stickComponent;
    stickComponent.reach = staleReach;
    stickComponent.width = staleReach;
    stickComponent.localOffset = {0.0f, 0.0f, staleReach};
    root.AddComponent<StickComponent>(stickComponent);

    const std::filesystem::path path = Paths::TempFile(fileName);
    HK_CHECK(static_cast<bool>(PrefabSerializer::Save(prefabScene, root, path)));
    return path;
}

void SetAllNormalSpawnPrefabs(Scene& scene, const std::filesystem::path& prefabPath) {
    auto view = scene.Registry().view<SpawnPointComponent>();
    for (const entt::entity handle : view) {
        SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (!spawn.faceoffSpawn) {
            spawn.playerPrefabPath = prefabPath;
        }
    }
}

Entity FindSingleStickChild(Scene& scene, Entity player) {
    Entity found;
    for (Entity child : scene.GetChildren(player)) {
        if (!child.HasComponent<StickComponent>()) {
            continue;
        }
        HK_CHECK(!found.IsValid());
        found = child;
    }
    return found;
}

void CheckRoleStick(Scene& scene,
                    Entity player,
                    const StickTuning& expected,
                    const std::filesystem::path& expectedPrefabPath) {
    HK_CHECK(player.HasComponent<StickComponent>());
    const StickComponent& playerStick = player.GetComponent<StickComponent>();
    HK_CHECK_EQ(playerStick.ownerPlayer, player.GetUUID());
    HK_CHECK_NEAR(playerStick.reach, expected.reach, 0.001f);
    HK_CHECK_NEAR(playerStick.width, expected.width, 0.001f);
    HK_CHECK_EQ(playerStick.localOffset, expected.localOffset);

    Entity stick = FindSingleStickChild(scene, player);
    HK_CHECK(stick.IsValid());
    if (!stick.IsValid()) {
        return;
    }
    HK_CHECK_EQ(stick.GetName(), std::string("Stick"));
    HK_CHECK(stick.HasComponent<PrefabComponent>());
    if (stick.HasComponent<PrefabComponent>()) {
        HK_CHECK_EQ(stick.GetComponent<PrefabComponent>().sourcePath, expectedPrefabPath);
    }
    HK_CHECK(stick.HasComponent<StickComponent>());
    if (stick.HasComponent<StickComponent>()) {
        const StickComponent& childStick = stick.GetComponent<StickComponent>();
        HK_CHECK_EQ(childStick.ownerPlayer, player.GetUUID());
        HK_CHECK_NEAR(childStick.reach, expected.reach, 0.001f);
        HK_CHECK_NEAR(childStick.width, expected.width, 0.001f);
        HK_CHECK_EQ(childStick.localOffset, expected.localOffset);
    }
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
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Warmup);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().periodCount, settings.periodCount);
    HK_CHECK_NEAR(match.GetComponent<MatchStateComponent>().periodTimeRemaining, 180.0f, 0.0001f);
    HK_CHECK_NEAR(match.GetComponent<MatchStateComponent>().phaseTimer, settings.pregameCountdownSeconds, 0.0001f);
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
    CheckAllPlayersFacePuck(scene);

    Entity homeGoal = scene.FindEntityByName("Home Goal");
    Entity awayGoal = scene.FindEntityByName("Away Goal");
    HK_CHECK_EQ(homeGoal.GetComponent<GoalGameplayComponent>().defendingTeam, GameplayTeam::Home);
    HK_CHECK_EQ(homeGoal.GetComponent<GoalGameplayComponent>().scoringTeam, GameplayTeam::Away);
    HK_CHECK_EQ(awayGoal.GetComponent<GoalGameplayComponent>().defendingTeam, GameplayTeam::Away);
    HK_CHECK_EQ(awayGoal.GetComponent<GoalGameplayComponent>().scoringTeam, GameplayTeam::Home);

    {
        Scene prefabSpawnScene("PrefabSpawnScene");
        BuildValidGameplayScene(prefabSpawnScene);
        const std::filesystem::path prefabPath = SavePlayerPrefab();
        const std::filesystem::path skaterStickPath =
            SaveStickPrefab("spawn_skater_stick.prefab.yaml", "Skater Stick Visual", 77.0f);
        const std::filesystem::path goalieStickPath =
            SaveStickPrefab("spawn_goalie_stick.prefab.yaml", "Goalie Stick Visual", 88.0f);
        SetAllNormalSpawnPrefabs(prefabSpawnScene, prefabPath);

        GameplayTuning tuning;
        tuning.skaterStick.prefabPath = skaterStickPath;
        tuning.skaterStick.reach = 1.75f;
        tuning.skaterStick.width = 0.30f;
        tuning.skaterStick.localOffset = {0.0f, 0.15f, 1.40f};
        tuning.goalieStick.prefabPath = goalieStickPath;
        tuning.goalieStick.reach = 2.35f;
        tuning.goalieStick.width = 0.45f;
        tuning.goalieStick.localOffset = {0.0f, 0.20f, 1.70f};

        GameplayWorld prefabWorld;
        HK_CHECK(static_cast<bool>(prefabWorld.Init(prefabSpawnScene, nullptr, settings, tuning)));
        Entity prefabPlayer = FindPlayer(prefabSpawnScene, PlayerSlot::HomeSkater0);
        HK_CHECK(prefabPlayer.IsValid());
        if (prefabPlayer.IsValid()) {
            HK_CHECK(prefabPlayer.HasComponent<PrefabComponent>());
            HK_CHECK(prefabPlayer.HasComponent<CameraRigMarkerComponent>());
            if (prefabPlayer.HasComponent<CameraRigMarkerComponent>()) {
                HK_CHECK_EQ(prefabPlayer.GetComponent<CameraRigMarkerComponent>().purpose,
                            std::string("PrefabSpawnMarker"));
            }
            HK_CHECK(prefabPlayer.GetComponent<PlayerComponent>().team == GameplayTeam::Home);
            HK_CHECK_NEAR(prefabPlayer.GetComponent<TransformComponent>().localPosition.z, -5.0f, 0.001f);
            HK_CHECK(prefabPlayer.HasComponent<PlayerRuntimeComponent>());
            HK_CHECK(prefabPlayer.HasComponent<StickComponent>());
        }

        auto players = prefabSpawnScene.Registry().view<PlayerComponent>();
        for (const entt::entity handle : players) {
            Entity player(handle, &prefabSpawnScene);
            const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
            if (playerComponent.role == GameplayRole::Goalie) {
                CheckRoleStick(prefabSpawnScene, player, tuning.goalieStick, goalieStickPath);
            } else {
                CheckRoleStick(prefabSpawnScene, player, tuning.skaterStick, skaterStickPath);
            }
        }
    }

    {
        Scene uniqueScene("UniqueSpawnPoolScene");
        BuildValidGameplayScene(uniqueScene);
        GameplaySettings uniqueSettings;
        uniqueSettings.spawnRandomSeed = 1234u;
        HK_CHECK(static_cast<bool>(MatchSystem::InitializeMatch(uniqueScene, uniqueSettings)));

        std::vector<glm::vec3> occupiedHomePositions;
        auto view = uniqueScene.Registry().view<PlayerComponent, TransformComponent>();
        for (const entt::entity handle : view) {
            const PlayerComponent& player = view.get<PlayerComponent>(handle);
            if (player.team == GameplayTeam::Home) {
                occupiedHomePositions.push_back(view.get<TransformComponent>(handle).localPosition);
            }
        }
        HK_CHECK_EQ(occupiedHomePositions.size(), static_cast<std::size_t>(4));
        for (std::size_t i = 0; i < occupiedHomePositions.size(); ++i) {
            for (std::size_t j = i + 1; j < occupiedHomePositions.size(); ++j) {
                HK_CHECK(occupiedHomePositions[i] != occupiedHomePositions[j]);
            }
        }
    }

    {
        for (uint32_t seed = 0; seed < 16; ++seed) {
            Scene roleMarkedScene("RoleMarkedSpawnScene");
            glm::vec3 homeGoalieSpawn{0.0f};
            glm::vec3 awayGoalieSpawn{0.0f};
            BuildRoleMarkedGameplayScene(roleMarkedScene, homeGoalieSpawn, awayGoalieSpawn);

            GameplaySettings roleMarkedSettings;
            roleMarkedSettings.spawnRandomSeed = seed;
            HK_CHECK(static_cast<bool>(MatchSystem::InitializeMatch(roleMarkedScene, roleMarkedSettings)));

            Entity homeGoalie = FindPlayer(roleMarkedScene, PlayerSlot::HomeGoalie);
            Entity awayGoalie = FindPlayer(roleMarkedScene, PlayerSlot::AwayGoalie);
            HK_CHECK(homeGoalie.IsValid());
            HK_CHECK(awayGoalie.IsValid());
            if (homeGoalie.IsValid()) {
                const glm::vec3& position = homeGoalie.GetComponent<TransformComponent>().localPosition;
                HK_CHECK_NEAR(position.x, homeGoalieSpawn.x, 0.001f);
                HK_CHECK_NEAR(position.z, homeGoalieSpawn.z, 0.001f);
            }
            if (awayGoalie.IsValid()) {
                const glm::vec3& position = awayGoalie.GetComponent<TransformComponent>().localPosition;
                HK_CHECK_NEAR(position.x, awayGoalieSpawn.x, 0.001f);
                HK_CHECK_NEAR(position.z, awayGoalieSpawn.z, 0.001f);
            }
        }
    }
}
