#include "Test.hpp"

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"

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

void RunPassingTests() {
    HockeyTest::BeginSuite("PassingTests");

    Scene scene("PassingScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 100.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

    Entity passer = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity teammate = FindPlayer(scene, PlayerSlot::HomeSkater1);
    Entity wideTeammate = FindPlayer(scene, PlayerSlot::HomeSkater2);
    Entity opponent = FindPlayer(scene, PlayerSlot::AwaySkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(passer.IsValid());
    HK_CHECK(teammate.IsValid());
    HK_CHECK(wideTeammate.IsValid());
    HK_CHECK(opponent.IsValid());
    HK_CHECK(puck.IsValid());

    passer.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    passer.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    teammate.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 8.0f};
    wideTeammate.GetComponent<TransformComponent>().localPosition = {8.0f, 0.0f, 0.0f};
    opponent.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 2.0f};
    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(passer);

    GameplayEventQueue events;
    HK_CHECK(PuckPossession::TryAcquire(scene, passer, puck, events));

    GameplayInputFrame input;
    input.playerIndex = passer.GetComponent<PlayerComponent>().playerIndex;
    input.inputSequence = 1;
    input.simulationTick = 1;
    input.aim = {0.0f, 1.0f};
    input.passPressed = true;
    world.PushInput(input);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    std::vector<GameplayEvent> passEvents = world.DrainEvents();

    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Passed);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
    HK_CHECK_EQ(passer.GetComponent<PassComponent>().targetPlayer, teammate.GetUUID());
    const float expectedPassSpeed =
        world.GetTuning().pass.power * (1.0f - world.GetTuning().puck.loosePuckDrag * settings.fixedDeltaSeconds);
    HK_CHECK_NEAR(glm::length(puck.GetComponent<PuckRuntimeComponent>().velocity), expectedPassSpeed, 0.001);
    HK_CHECK_NEAR(glm::normalize(puck.GetComponent<PuckRuntimeComponent>().velocity).z, 1.0f, 0.001);
    HK_CHECK(SawEvent(passEvents, GameplayEventType::PuckPassed));
}
