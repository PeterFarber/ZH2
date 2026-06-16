#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
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
    rink.AddComponent<PlayAreaComponent>();

    AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f}).AddComponent<PuckComponent>();
    AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Home;
    AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Away;
    AddMarker(scene, "Center Faceoff", {0.0f, 0.0f, 0.0f}).AddComponent<FaceoffSpotComponent>().index = 0;

    for (int i = 0; i < 3; ++i) {
        AddSpawn(scene, Team::Home, PlayerRole::Skater, i, {-3.0f + static_cast<float>(i), 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, PlayerRole::Skater, i, {-3.0f + static_cast<float>(i), 0.0f, 5.0f});
    }
    AddSpawn(scene, Team::Home, PlayerRole::Goalie, 0, {0.0f, 0.0f, -24.0f});
    AddSpawn(scene, Team::Away, PlayerRole::Goalie, 0, {0.0f, 0.0f, 24.0f});
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

void RunGameplayWorldTests() {
    HockeyTest::BeginSuite("GameplayWorldTests");

    Scene scene("GameplayWorldScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 1.2f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    HK_CHECK(world.IsInitialized());
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::MatchInitialized));

    Entity match = scene.FindEntityByName("Match State");
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::FaceoffSetup);

    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Faceoff);
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::FaceoffStarted));

    for (int i = 0; i < 61; ++i) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, static_cast<uint64_t>(2 + i));
    }
    std::vector<GameplayEvent> faceoffEvents = world.DrainEvents();
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Playing);
    HK_CHECK(match.GetComponent<MatchStateComponent>().clockRunning);
    HK_CHECK(SawEvent(faceoffEvents, GameplayEventType::FaceoffEnded));
    HK_CHECK(SawEvent(faceoffEvents, GameplayEventType::PeriodStarted));

    for (int i = 0; i < 80; ++i) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, static_cast<uint64_t>(100 + i));
    }
    std::vector<GameplayEvent> endEvents = world.DrainEvents();
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::PeriodEnded);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().period, 2u);
    HK_CHECK(SawEvent(endEvents, GameplayEventType::PeriodEnded));

    match.GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
    match.GetComponent<MatchStateComponent>().clockRunning = true;
    match.GetComponent<MatchStateComponent>().period = match.GetComponent<MatchStateComponent>().periodCount;
    match.GetComponent<MatchStateComponent>().periodTimeRemaining = settings.fixedDeltaSeconds;
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1000);
    std::vector<GameplayEvent> matchEndEvents = world.DrainEvents();
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::MatchEnded);
    HK_CHECK(SawEvent(matchEndEvents, GameplayEventType::MatchEnded));

    GameplayInputFrame input;
    input.playerIndex = 0;
    input.inputSequence = 1;
    input.simulationTick = 1001;
    input.move = {1.0f, 0.0f};
    world.PushInput(input);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1001);

    world.Shutdown();
    HK_CHECK(!world.IsInitialized());
}
