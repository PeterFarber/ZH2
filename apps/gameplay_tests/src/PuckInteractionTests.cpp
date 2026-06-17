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

} // namespace

void RunStickHandlingTests() {
    HockeyTest::BeginSuite("StickHandlingTests");

    Scene scene("StickHandlingScene");
    BuildValidGameplayScene(scene);

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
    world.DrainEvents();

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(skater.IsValid());
    HK_CHECK(puck.IsValid());
    HK_CHECK_EQ(skater.GetComponent<StickComponent>().ownerPlayer, skater.GetUUID());

    skater.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, 3.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = {1.0f, 0.0f, 0.0f};
    const glm::vec3 expectedStickPosition{3.0f, 0.0f, 3.0f};
    HK_CHECK_EQ(StickHandling::GetStickWorldPosition(skater), expectedStickPosition);

    puck.GetComponent<TransformComponent>().localPosition = expectedStickPosition;
    HK_CHECK(StickHandling::CanControlPuck(skater, puck));

    puck.GetComponent<TransformComponent>().localPosition = {20.0f, 0.0f, 20.0f};
    HK_CHECK(!StickHandling::CanControlPuck(skater, puck));
}

void RunPuckInteractionTests() {
    HockeyTest::BeginSuite("PuckInteractionTests");

    Scene scene("PuckInteractionScene");
    BuildValidGameplayScene(scene);

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
    world.DrainEvents();

    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
    Entity home = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity away = FindPlayer(scene, PlayerSlot::AwaySkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(home.IsValid());
    HK_CHECK(away.IsValid());
    HK_CHECK(puck.IsValid());

    home.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    home.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    away.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.25f};
    away.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(home);

    GameplayEventQueue events;
    HK_CHECK(PuckPossession::TryAcquire(scene, home, puck, events));
    HK_CHECK(SawEvent(events.Drain(), GameplayEventType::PuckPossessionChanged));
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, home.GetUUID());
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().lastTouchedPlayer, home.GetUUID());
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().lastTouchedTeam, GameplayTeam::Home);
    HK_CHECK(home.GetComponent<SkaterComponent>().hasPuck);

    HK_CHECK(!PuckPossession::TryAcquire(scene, away, puck, events));
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, home.GetUUID());

    home.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, 0.0f};
    PuckPossession::FixedUpdate(scene, events);
    HK_CHECK_EQ(puck.GetComponent<TransformComponent>().localPosition, StickHandling::GetStickWorldPosition(home));

    PuckPossession::Release(scene, puck, events);
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
    HK_CHECK(!home.GetComponent<SkaterComponent>().hasPuck);
    HK_CHECK(SawEvent(events.Drain(), GameplayEventType::PuckPossessionChanged));
}
