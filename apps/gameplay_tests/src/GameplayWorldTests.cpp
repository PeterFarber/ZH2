#include "Test.hpp"

#include <entt/entt.hpp>

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

bool SawEvent(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

int CountEvents(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    int count = 0;
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            ++count;
        }
    }
    return count;
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

} // namespace

void RunGameplayWorldTests() {
    HockeyTest::BeginSuite("GameplayWorldTests");

    Scene scene("GameplayWorldScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 1.2f;
    settings.pregameCountdownSeconds = 2.0f;
    settings.countdownBeepStartSeconds = 1.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    HK_CHECK(world.IsInitialized());
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::MatchInitialized));

    Entity match = scene.FindEntityByName("Match State");
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Warmup);
    HK_CHECK_NEAR(match.GetComponent<MatchStateComponent>().phaseTimer, 2.0f, 0.0001f);

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    GameplayInputFrame warmupInput;
    warmupInput.playerIndex = skater.GetComponent<PlayerComponent>().playerIndex;
    warmupInput.inputSequence = 1;
    warmupInput.simulationTick = 1;
    warmupInput.move = {1.0f, 0.0f};
    world.PushInput(warmupInput);
    world.FixedUpdate(scene, 1.0f, 1);
    std::vector<GameplayEvent> countdownEvents = world.DrainEvents();
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Warmup);
    HK_CHECK_EQ(skater.GetComponent<PlayerRuntimeComponent>().velocity, glm::vec3{0.0f});
    HK_CHECK_EQ(CountEvents(countdownEvents, GameplayEventType::CountdownTick), 1);
    HK_CHECK_EQ(CountEvents(countdownEvents, GameplayEventType::CountdownBeep), 1);

    world.FixedUpdate(scene, 1.0f, 2);
    std::vector<GameplayEvent> countdownEndEvents = world.DrainEvents();
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::FaceoffSetup);
    HK_CHECK_EQ(CountEvents(countdownEndEvents, GameplayEventType::CountdownTick), 1);
    HK_CHECK_EQ(CountEvents(countdownEndEvents, GameplayEventType::CountdownBeep), 1);

    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 3);
    HK_CHECK_EQ(match.GetComponent<MatchStateComponent>().phase, MatchPhase::Faceoff);
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::FaceoffStarted));

    for (int i = 0; i < 61; ++i) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, static_cast<uint64_t>(4 + i));
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
