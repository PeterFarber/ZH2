#include "Test.hpp"

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

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

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
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

void SetPlaying(Scene& scene) {
    MatchStateComponent& match = FindMatch(scene).GetComponent<MatchStateComponent>();
    match.phase = MatchPhase::Playing;
    match.clockRunning = true;
}

void PushMove(GameplayWorld& world, uint32_t playerIndex, uint64_t tick, const glm::vec2& move, bool boost = false) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.move = move;
    input.aim = move;
    input.boostPressed = boost;
    world.PushInput(input);
}

void PushWaypoint(GameplayWorld& world,
                  uint32_t playerIndex,
                  uint64_t tick,
                  const glm::vec3& target,
                  bool boost = false,
                  bool brake = false,
                  bool quickTurn = false) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.moveTarget = target;
    input.setMoveTarget = true;
    input.boostPressed = boost;
    input.brakePressed = brake;
    input.brakeHeld = brake;
    input.quickTurnPressed = quickTurn;
    world.PushInput(input);
}

void PushBrake(GameplayWorld& world, uint32_t playerIndex, uint64_t tick) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.brakePressed = true;
    input.brakeHeld = true;
    input.clearMoveTarget = true;
    world.PushInput(input);
}

void PushGoalieShield(GameplayWorld& world, uint32_t playerIndex, uint64_t tick) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.goalieShieldPressed = true;
    world.PushInput(input);
}

} // namespace

void RunSkaterMovementTests() {
    HockeyTest::BeginSuite("SkaterMovementTests");

    Scene scene("SkaterMovementScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 100.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    SetPlaying(scene);

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    HK_CHECK(skater.IsValid());
    const uint32_t playerIndex = skater.GetComponent<PlayerComponent>().playerIndex;

    PushMove(world, playerIndex, 1, {1.0f, 0.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    HK_CHECK(skater.GetComponent<PlayerRuntimeComponent>().velocity.x > 0.0f);
    HK_CHECK(skater.GetComponent<TransformComponent>().localPosition.x > -3.0f);
    HK_CHECK_NEAR(skater.GetComponent<PlayerRuntimeComponent>().facingDirection.x, 1.0f, 0.001);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    skater.GetComponent<TransformComponent>().localPosition = {-3.0f, 0.0f, -5.0f};
    PushWaypoint(world, playerIndex, 2, {3.0f, 0.0f, -5.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 2);
    HK_CHECK(skater.GetComponent<PlayerRuntimeComponent>().hasMoveTarget);
    HK_CHECK(skater.GetComponent<PlayerRuntimeComponent>().velocity.x > 0.0f);
    HK_CHECK_NEAR(skater.GetComponent<PlayerRuntimeComponent>().facingDirection.x, 1.0f, 0.001);

    const float speedAfterInput = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    GameplayInputFrame brakeInput;
    brakeInput.playerIndex = playerIndex;
    brakeInput.inputSequence = 3;
    brakeInput.simulationTick = 3;
    brakeInput.brakePressed = true;
    brakeInput.brakeHeld = true;
    brakeInput.clearMoveTarget = true;
    world.PushInput(brakeInput);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 3);
    const float speedAfterNoInput = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(speedAfterNoInput < speedAfterInput);
    HK_CHECK(!skater.GetComponent<PlayerRuntimeComponent>().hasMoveTarget);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{4.0f, 0.0f, 0.0f};
    PushBrake(world, playerIndex, 4);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 4);
    HK_CHECK_EQ(skater.GetComponent<PlayerRuntimeComponent>().velocity, glm::vec3{0.0f});

    for (uint64_t tick = 6; tick < 90; ++tick) {
        PushMove(world, playerIndex, tick, {1.0f, 0.0f});
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    const float clampedSpeed = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(clampedSpeed <= skater.GetComponent<SkaterComponent>().maxSpeed + 0.001f);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = glm::vec3{1.0f, 0.0f, 0.0f};
    PushMove(world, playerIndex, 90, {0.0f, 0.0f}, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 90);
    const float boostedSpeed = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(boostedSpeed >= world.GetTuning().skater.boostImpulse * 0.9f);
    HK_CHECK(skater.GetComponent<PlayerRuntimeComponent>().boostCooldown > 0.0f);
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::PlayerBoosted));

    PushMove(world, playerIndex, 91, {0.0f, 0.0f}, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 91);
    HK_CHECK(glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity) <= boostedSpeed);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{4.0f, 0.0f, 0.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = glm::vec3{1.0f, 0.0f, 0.0f};
    PushWaypoint(world, playerIndex, 150, {10.0f, 0.0f, -5.0f}, false, false, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 150);
    HK_CHECK(!skater.GetComponent<PlayerRuntimeComponent>().hasMoveTarget);
    HK_CHECK(glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity) < 4.0f);
    HK_CHECK_NEAR(skater.GetComponent<PlayerRuntimeComponent>().facingDirection.x, -1.0f, 0.001);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    skater.GetComponent<PlayerRuntimeComponent>().movementEnabled = false;
    PushMove(world, playerIndex, 200, {1.0f, 0.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 200);
    const glm::vec3 zero{0.0f};
    HK_CHECK_EQ(skater.GetComponent<PlayerRuntimeComponent>().velocity, zero);
}

void RunGoalieMovementTests() {
    HockeyTest::BeginSuite("GoalieMovementTests");

    Scene scene("GoalieMovementScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 100.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    SetPlaying(scene);

    Entity goalie = FindPlayer(scene, PlayerSlot::HomeGoalie);
    HK_CHECK(goalie.IsValid());
    const uint32_t playerIndex = goalie.GetComponent<PlayerComponent>().playerIndex;
    const float startZ = goalie.GetComponent<TransformComponent>().localPosition.z;

    PushMove(world, playerIndex, 1, {1.0f, 1.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    HK_CHECK(goalie.GetComponent<PlayerRuntimeComponent>().velocity.x > 0.0f);
    HK_CHECK_NEAR(goalie.GetComponent<PlayerRuntimeComponent>().velocity.z, 0.0f, 0.001);
    HK_CHECK_NEAR(goalie.GetComponent<TransformComponent>().localPosition.z, startZ, 0.001);

    for (uint64_t tick = 2; tick < 90; ++tick) {
        PushMove(world, playerIndex, tick, {1.0f, 0.0f});
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    const float goalieSpeed = glm::length(goalie.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(goalieSpeed <= goalie.GetComponent<GoalieComponent>().maxSpeed + 0.001f);
    HK_CHECK(goalie.GetComponent<GoalieComponent>().maxSpeed < FindPlayer(scene, PlayerSlot::HomeSkater0).GetComponent<SkaterComponent>().maxSpeed);

    goalie.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    goalie.GetComponent<PlayerRuntimeComponent>().facingDirection = glm::vec3{1.0f, 0.0f, 0.0f};
    HK_CHECK_EQ(goalie.GetComponent<PlayerRuntimeComponent>().goalieBoostCharges, 2u);
    PushMove(world, playerIndex, 90, {0.0f, 0.0f}, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 90);
    HK_CHECK_EQ(goalie.GetComponent<PlayerRuntimeComponent>().goalieBoostCharges, 1u);
    HK_CHECK(glm::length(goalie.GetComponent<PlayerRuntimeComponent>().velocity) >= world.GetTuning().goalie.boostImpulse * 0.9f);

    for (uint64_t tick = 91; tick < 331; ++tick) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    HK_CHECK_EQ(goalie.GetComponent<PlayerRuntimeComponent>().goalieBoostCharges, 2u);

    Entity puck = FindPuck(scene);
    Entity awaySkater = FindPlayer(scene, PlayerSlot::AwaySkater0);
    goalie.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, -24.0f};
    goalie.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    puck.GetComponent<TransformComponent>().localPosition = {1.0f, 0.0f, -24.0f};
    puck.GetComponent<PuckGameplayComponent>().state = PuckState::Loose;
    puck.GetComponent<PuckGameplayComponent>().possessingPlayer = {};
    puck.GetComponent<PuckRuntimeComponent>().velocity = {-2.0f, 0.0f, 0.0f};
    awaySkater.GetComponent<TransformComponent>().localPosition = {0.5f, 0.0f, -24.0f};
    awaySkater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};

    PushGoalieShield(world, playerIndex, 331);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 331);
    HK_CHECK(goalie.GetComponent<PlayerRuntimeComponent>().shieldActive);
    HK_CHECK(goalie.GetComponent<PlayerRuntimeComponent>().shieldTimer > 0.0f);
    HK_CHECK(goalie.GetComponent<PlayerRuntimeComponent>().shieldCooldown > 0.0f);
    HK_CHECK(puck.GetComponent<PuckRuntimeComponent>().velocity.x > 0.0f);
    HK_CHECK(glm::length(puck.GetComponent<PuckRuntimeComponent>().velocity) > 10.0f);
    HK_CHECK(glm::length(awaySkater.GetComponent<PlayerRuntimeComponent>().velocity) > 0.0f);
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::GoalieShieldStarted));

    for (uint64_t tick = 332; tick < 400; ++tick) {
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    HK_CHECK(!goalie.GetComponent<PlayerRuntimeComponent>().shieldActive);
    HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::GoalieShieldEnded));
}
