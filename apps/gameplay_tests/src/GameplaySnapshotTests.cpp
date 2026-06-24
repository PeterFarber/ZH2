#include "Test.hpp"

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Simulation/GameplaySnapshot.hpp"
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

} // namespace

void RunGameplaySnapshotTests() {
    HockeyTest::BeginSuite("GameplaySnapshotTests");

    Scene scene("GameplaySnapshotScene");
    BuildValidGameplayScene(scene);

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
    world.DrainEvents();

    Entity match = FindMatch(scene);
    match.GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
    match.GetComponent<MatchStateComponent>().homeScore = 2;
    match.GetComponent<MatchStateComponent>().awayScore = 1;

    Entity player = FindPlayer(scene, PlayerSlot::HomeSkater0);
    player.GetComponent<PlayerRuntimeComponent>().velocity = {1.0f, 0.0f, 0.0f};
    player.GetComponent<SkaterComponent>().hasPuck = true;
    player.GetComponent<ShotComponent>().charge = 0.5f;
    player.GetComponent<ShotComponent>().charging = true;

    Entity puck = FindPuck(scene);
    puck.GetComponent<PuckGameplayComponent>().state = PuckState::Possessed;
    puck.GetComponent<PuckGameplayComponent>().possessingPlayer = player.GetUUID();
    puck.GetComponent<PuckRuntimeComponent>().velocity = {2.0f, 0.0f, 0.0f};

    GameplaySnapshot snapshot = BuildGameplaySnapshot(scene, 42);
    HK_CHECK_EQ(snapshot.tick, 42ull);
    HK_CHECK_EQ(snapshot.match.phase, MatchPhase::Playing);
    HK_CHECK_EQ(snapshot.match.homeScore, 2u);
    HK_CHECK_EQ(snapshot.match.awayScore, 1u);
    HK_CHECK_EQ(snapshot.players.size(), static_cast<std::size_t>(8));
    HK_CHECK_EQ(snapshot.players.front().playerIndex, 0u);
    HK_CHECK(snapshot.players.front().hasPuck);
    HK_CHECK(snapshot.players.front().shotCharging);
    HK_CHECK_NEAR(snapshot.players.front().shotChargeRatio, 0.5f, 0.0001f);
    HK_CHECK_EQ(snapshot.puck.entity, puck.GetUUID());
    HK_CHECK_EQ(snapshot.puck.state, PuckState::Possessed);
    HK_CHECK_EQ(snapshot.puck.possessingPlayer, player.GetUUID());
}
