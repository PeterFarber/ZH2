#include "Test.hpp"

#include <filesystem>
#include <string>
#include <type_traits>

#include <glm/glm.hpp>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Match/FaceoffSystem.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"

using namespace Hockey;

namespace {

template <typename T, typename = void>
struct HasAllowBodyChecking : std::false_type {};
template <typename T>
struct HasAllowBodyChecking<T, std::void_t<decltype(&T::allowBodyChecking)>> : std::true_type {};

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

void BuildMinimalGameplayScene(Scene& scene) {
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

} // namespace

void RunSettingsTuningTests() {
    HockeyTest::BeginSuite("SettingsTuningTests");

    Config config;
    GameplaySettings defaults = LoadGameplaySettings(config);
    HK_CHECK(defaults.enabled);
    HK_CHECK_EQ(defaults.targetPlayerCount, 8u);
    HK_CHECK_EQ(defaults.skatersPerTeam, 3u);
    HK_CHECK_EQ(defaults.goaliesPerTeam, 1u);
    HK_CHECK_NEAR(defaults.fixedDeltaSeconds, 1.0f / 60.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.periodLengthSeconds, 180.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.pregameCountdownSeconds, 10.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.countdownBeepStartSeconds, 4.0f, 0.0001f);
    HK_CHECK_EQ(defaults.spawnRandomSeed, 0x5A02024u);
    HK_CHECK_EQ(defaults.waypointPrefabPath.generic_string(),
                std::string("data/raw/prefabs/Waypoint_Marker.prefab.yaml"));

    GameplaySettings source;
    source.enabled = false;
    source.targetPlayerCount = 6;
    source.periodLengthSeconds = 120.0f;
    source.pregameCountdownSeconds = 7.0f;
    source.countdownBeepStartSeconds = 3.0f;
    source.spawnRandomSeed = 424242u;
    source.waypointPrefabPath = "data/raw/prefabs/CustomWaypoint.prefab.yaml";
    source.logGameplayEvents = true;
    SaveGameplaySettings(config, source);
    GameplaySettings loaded = LoadGameplaySettings(config);
    HK_CHECK(!loaded.enabled);
    HK_CHECK_EQ(loaded.targetPlayerCount, 6u);
    HK_CHECK_NEAR(loaded.periodLengthSeconds, 120.0f, 0.0001f);
    HK_CHECK_NEAR(loaded.pregameCountdownSeconds, 7.0f, 0.0001f);
    HK_CHECK_NEAR(loaded.countdownBeepStartSeconds, 3.0f, 0.0001f);
    HK_CHECK_EQ(loaded.spawnRandomSeed, 424242u);
    HK_CHECK_EQ(loaded.waypointPrefabPath.generic_string(),
                std::string("data/raw/prefabs/CustomWaypoint.prefab.yaml"));
    HK_CHECK(loaded.logGameplayEvents);
    HK_CHECK(!HasAllowBodyChecking<GameplaySettings>::value);
    HK_CHECK(!config.Has("gameplay.allow_body_checking"));

    GameplaySettings rulesSource;
    rulesSource.faceoffDelaySeconds = 2.25f;
    rulesSource.goalDetectionRadius = 1.75f;
    rulesSource.requirePuckForGoal = false;
    SaveGameplaySettings(config, rulesSource);
    GameplaySettings rulesLoaded = LoadGameplaySettings(config);
    HK_CHECK_NEAR(rulesLoaded.faceoffDelaySeconds, 2.25f, 0.0001f);
    HK_CHECK_NEAR(rulesLoaded.goalDetectionRadius, 1.75f, 0.0001f);
    HK_CHECK(!rulesLoaded.requirePuckForGoal);

    GameplayTuning suppliedTuning;
    suppliedTuning.skater.stealCooldownSeconds = 0.45f;
    GameplayWorld world;
    Scene tuningScene("SuppliedTuningScene");
    BuildMinimalGameplayScene(tuningScene);
    GameplaySettings disabledSettings;
    disabledSettings.enabled = false;
    HK_CHECK(static_cast<bool>(world.Init(tuningScene, nullptr, disabledSettings, suppliedTuning)));
    HK_CHECK_NEAR(world.GetTuning().skater.stealCooldownSeconds, 0.45f, 0.0001f);
    world.Shutdown();

    Scene faceoffScene("FaceoffTimingSettingsScene");
    Entity match = faceoffScene.CreateEntity("Match");
    MatchStateComponent& state = match.AddComponent<MatchStateComponent>();
    state.phase = MatchPhase::FaceoffSetup;
    GameplayEventQueue events;
    GameplaySettings faceoffSettings;
    faceoffSettings.faceoffDelaySeconds = 2.0f;

    FaceoffSystem::FixedUpdate(faceoffScene, 1.0f, faceoffSettings, events);
    HK_CHECK_EQ(state.phase, MatchPhase::Faceoff);
    FaceoffSystem::FixedUpdate(faceoffScene, 1.0f, faceoffSettings, events);
    HK_CHECK_EQ(state.phase, MatchPhase::Faceoff);
    FaceoffSystem::FixedUpdate(faceoffScene, 1.0f, faceoffSettings, events);
    HK_CHECK_EQ(state.phase, MatchPhase::Playing);

    const Result<GameplayTuning> tuning = TuningSerializer::Load(Paths::DataFile("gameplay/tuning.default.yaml"));
    HK_CHECK_MSG(static_cast<bool>(tuning), tuning.error);
    if (tuning) {
        HK_CHECK_NEAR(tuning.value.skater.maxSpeed, 7.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.boostImpulse, 6.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.boostCooldownSeconds, 1.25f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.slideStopDamping, 0.35f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.doubleStopWindowSeconds, 0.30f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.stealRadius, 1.5f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.stealCooldownSeconds, 0.35f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.maxSpeed, 6.5f, 0.0001f);
        HK_CHECK_EQ(tuning.value.goalie.boostCharges, 2u);
        HK_CHECK_NEAR(tuning.value.goalie.boostRechargeSeconds, 4.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.boostImpulse, 8.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldRadius, 2.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldDurationSeconds, 1.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldCooldownSeconds, 5.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldReflectImpulse, 22.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.puck.floorY, 0.05f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.puck.possessionOffset.z, 2.98f, 0.0001f);
        HK_CHECK_EQ(tuning.value.skaterStick.prefabPath.generic_string(),
                    std::string("data/raw/prefabs/Skater_Stick.prefab.yaml"));
        HK_CHECK_NEAR(tuning.value.skaterStick.reach, 1.5f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skaterStick.width, 0.25f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skaterStick.localOffset.z, 1.3f, 0.0001f);
        HK_CHECK_EQ(tuning.value.goalieStick.prefabPath.generic_string(),
                    std::string("data/raw/prefabs/Goalie_Stick.prefab.yaml"));
        HK_CHECK_NEAR(tuning.value.goalieStick.reach, 2.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalieStick.width, 0.35f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalieStick.localOffset.z, 1.55f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.shot.maxPower, 32.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.shot.selfCollisionGraceSeconds, 0.20f, 0.0001f);

        const std::string text = TuningSerializer::Serialize(tuning.value);
        HK_CHECK(text.find("PokeCheckCooldown") == std::string::npos);
        HK_CHECK(text.find("Pass:") == std::string::npos);
        HK_CHECK(text.find("Check:") == std::string::npos);
        HK_CHECK(text.find("StealRadius") != std::string::npos);
        HK_CHECK(text.find("StealCooldownSeconds") != std::string::npos);
        HK_CHECK(text.find("SkaterStick:") != std::string::npos);
        HK_CHECK(text.find("GoalieStick:") != std::string::npos);
        HK_CHECK(text.find("PrefabPath") != std::string::npos);
        HK_CHECK(text.find("Shot:") != std::string::npos);
        GameplayTuning roundTrip;
        HK_CHECK(TuningSerializer::Deserialize(text, roundTrip));
        HK_CHECK_NEAR(roundTrip.skater.boostImpulse, 6.0f, 0.0001f);
        HK_CHECK_EQ(roundTrip.goalie.boostCharges, 2u);
        HK_CHECK_NEAR(roundTrip.goalie.shieldReflectImpulse, 22.0f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.puck.floorY, 0.05f, 0.0001f);
        HK_CHECK_EQ(roundTrip.skaterStick.prefabPath.generic_string(),
                    std::string("data/raw/prefabs/Skater_Stick.prefab.yaml"));
        HK_CHECK_NEAR(roundTrip.skaterStick.localOffset.z, 1.3f, 0.0001f);
        HK_CHECK_EQ(roundTrip.goalieStick.prefabPath.generic_string(),
                    std::string("data/raw/prefabs/Goalie_Stick.prefab.yaml"));
        HK_CHECK_NEAR(roundTrip.goalieStick.reach, 2.0f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.shot.selfCollisionGraceSeconds, 0.20f, 0.0001f);
    }

    const char* roleStickYaml =
        "SkaterStick:\n"
        "  PrefabPath: data/raw/prefabs/TestSkaterStick.prefab.yaml\n"
        "  Reach: 1.75\n"
        "  Width: 0.3\n"
        "  LocalOffset: [0.1, 0.2, 1.4]\n"
        "GoalieStick:\n"
        "  PrefabPath: data/raw/prefabs/TestGoalieStick.prefab.yaml\n"
        "  Reach: 2.4\n"
        "  Width: 0.45\n"
        "  LocalOffset: [-0.1, 0.3, 1.8]\n";
    GameplayTuning roleStickTuning;
    HK_CHECK(TuningSerializer::Deserialize(roleStickYaml, roleStickTuning));
    HK_CHECK_EQ(roleStickTuning.skaterStick.prefabPath.generic_string(),
                std::string("data/raw/prefabs/TestSkaterStick.prefab.yaml"));
    HK_CHECK_NEAR(roleStickTuning.skaterStick.reach, 1.75f, 0.0001f);
    HK_CHECK_NEAR(roleStickTuning.skaterStick.width, 0.3f, 0.0001f);
    HK_CHECK_NEAR(roleStickTuning.skaterStick.localOffset.x, 0.1f, 0.0001f);
    HK_CHECK_EQ(roleStickTuning.goalieStick.prefabPath.generic_string(),
                std::string("data/raw/prefabs/TestGoalieStick.prefab.yaml"));
    HK_CHECK_NEAR(roleStickTuning.goalieStick.reach, 2.4f, 0.0001f);
    HK_CHECK_NEAR(roleStickTuning.goalieStick.width, 0.45f, 0.0001f);
    HK_CHECK_NEAR(roleStickTuning.goalieStick.localOffset.z, 1.8f, 0.0001f);
}
