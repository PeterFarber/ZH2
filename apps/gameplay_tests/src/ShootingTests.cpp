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

void PushShoot(GameplayWorld& world, uint32_t playerIndex, uint64_t tick, bool held, bool released) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.aim = {1.0f, 0.0f};
    input.shootHeld = held;
    input.shootReleased = released;
    world.PushInput(input);
}

} // namespace

void RunShootingTests() {
    HockeyTest::BeginSuite("ShootingTests");

    Scene scene("ShootingScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 100.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(skater.IsValid());
    HK_CHECK(puck.IsValid());

    skater.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(skater);

    GameplayEventQueue events;
    HK_CHECK(PuckPossession::TryAcquire(scene, skater, puck, events));
    events.Clear();

    const uint32_t playerIndex = skater.GetComponent<PlayerComponent>().playerIndex;
    PushShoot(world, playerIndex, 1, true, false);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    HK_CHECK(skater.GetComponent<ShotComponent>().charging);
    HK_CHECK(skater.GetComponent<ShotComponent>().charge > 0.0f);

    for (uint64_t tick = 2; tick < 90; ++tick) {
        PushShoot(world, playerIndex, tick, true, false);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    HK_CHECK_NEAR(skater.GetComponent<ShotComponent>().charge, world.GetTuning().shot.chargeSeconds, 0.001);

    PushShoot(world, playerIndex, 90, false, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 90);
    std::vector<GameplayEvent> shotEvents = world.DrainEvents();

    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Shot);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
    HK_CHECK(!skater.GetComponent<SkaterComponent>().hasPuck);
    HK_CHECK_NEAR(glm::length(puck.GetComponent<PuckRuntimeComponent>().velocity), world.GetTuning().shot.maxPower, 0.001);
    HK_CHECK_NEAR(glm::normalize(puck.GetComponent<PuckRuntimeComponent>().velocity).x, 1.0f, 0.001);
    HK_CHECK(!skater.GetComponent<ShotComponent>().charging);
    HK_CHECK_NEAR(skater.GetComponent<ShotComponent>().charge, 0.0f, 0.001);
    HK_CHECK(SawEvent(shotEvents, GameplayEventType::PuckShot));
}
