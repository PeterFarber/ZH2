#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Validation/GameplayValidation.hpp"

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

void BuildHeadlessServerScene(Scene& scene) {
    scene.SetMode(SceneMode::Server);

    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>();
    rink.AddComponent<PlayAreaComponent>();

    Entity puck = AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f});
    puck.AddComponent<PuckComponent>();
    puck.AddComponent<PuckGameplayComponent>();
    puck.AddComponent<PuckRuntimeComponent>();

    Entity homeGoal = AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f});
    homeGoal.AddComponent<GoalComponent>().defendingTeam = Team::Home;
    GoalGameplayComponent homeGameplayGoal;
    homeGameplayGoal.defendingTeam = GameplayTeam::Home;
    homeGameplayGoal.scoringTeam = GameplayTeam::Away;
    homeGoal.AddComponent<GoalGameplayComponent>(homeGameplayGoal);

    Entity awayGoal = AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f});
    awayGoal.AddComponent<GoalComponent>().defendingTeam = Team::Away;
    GoalGameplayComponent awayGameplayGoal;
    awayGameplayGoal.defendingTeam = GameplayTeam::Away;
    awayGameplayGoal.scoringTeam = GameplayTeam::Home;
    awayGoal.AddComponent<GoalGameplayComponent>(awayGameplayGoal);

    for (int i = 0; i < 4; ++i) {
        AddSpawn(scene, Team::Home, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, 5.0f});
    }
    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, true);
    }
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

void RunHeadlessServerGameplayTests() {
    HockeyTest::BeginSuite("HeadlessServerGameplayTests");

    RegisterGameplayComponents();

    Scene scene("HeadlessServerScene");
    BuildHeadlessServerScene(scene);

    const std::vector<SceneValidationIssue> issues = SceneValidator::Validate(scene);
    HK_CHECK_MSG(!SceneValidator::HasErrors(issues), "headless server gameplay scene validates");

    GameplaySettings settings;
    settings.periodLengthSeconds = 10.0f;
    settings.pregameCountdownSeconds = 0.0f;
    settings.logGameplayEvents = true;

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "server scene initializes gameplay");
    HK_CHECK(world.IsInitialized());
    HK_CHECK_EQ(scene.GetMode(), SceneMode::Server);

    for (uint64_t tick = 1; tick <= 120; ++tick) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }

    Entity match = scene.FindEntityByName("Match State");
    HK_CHECK(match.IsValid());
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Playing);
    HK_CHECK(match.GetComponent<MatchStateComponent>().clockRunning);

    Entity puck = scene.FindEntityByName("Puck");
    Entity homeGoal = scene.FindEntityByName("Home Goal");
    puck.GetComponent<TransformComponent>().localPosition = homeGoal.GetComponent<TransformComponent>().localPosition;

    world.DrainEvents();
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 121);
    const std::vector<GameplayEvent> goalEvents = world.DrainEvents();

    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().awayScore, 1u);
    HK_CHECK(SawEvent(goalEvents, GameplayEventType::GoalScored));
    HK_CHECK(SawEvent(goalEvents, GameplayEventType::ScoreChanged));

    world.Shutdown();
    HK_CHECK(!world.IsInitialized());
}
