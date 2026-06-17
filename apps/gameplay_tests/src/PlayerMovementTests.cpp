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

void SetPlaying(Scene& scene) {
    MatchStateComponent& match = FindMatch(scene).GetComponent<MatchStateComponent>();
    match.phase = MatchPhase::Playing;
    match.clockRunning = true;
}

void PushMove(GameplayWorld& world, uint32_t playerIndex, uint64_t tick, const glm::vec2& move, bool sprint = false) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.move = move;
    input.aim = move;
    input.sprint = sprint;
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

    const float speedAfterInput = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 2);
    const float speedAfterNoInput = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(speedAfterNoInput < speedAfterInput);

    for (uint64_t tick = 3; tick < 90; ++tick) {
        PushMove(world, playerIndex, tick, {1.0f, 0.0f});
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    const float clampedSpeed = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(clampedSpeed <= skater.GetComponent<SkaterComponent>().maxSpeed + 0.001f);

    skater.GetComponent<PlayerRuntimeComponent>().velocity = glm::vec3{0.0f};
    for (uint64_t tick = 90; tick < 150; ++tick) {
        PushMove(world, playerIndex, tick, {1.0f, 0.0f}, true);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    const float sprintSpeed = glm::length(skater.GetComponent<PlayerRuntimeComponent>().velocity);
    HK_CHECK(sprintSpeed > skater.GetComponent<SkaterComponent>().maxSpeed);

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
}
