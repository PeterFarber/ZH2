#include "Test.hpp"

#include <filesystem>
#include <string>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Waypoint/WaypointMarkerComponents.hpp"

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

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
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

void SetPlaying(Scene& scene) {
    MatchStateComponent& match = FindMatch(scene).GetComponent<MatchStateComponent>();
    match.phase = MatchPhase::Playing;
    match.clockRunning = true;
}

std::filesystem::path SaveWaypointPrefab() {
    Scene prefabScene("WaypointPrefabScene");
    Entity root = prefabScene.CreateEntity("Visible Waypoint Marker");
    root.GetComponent<TransformComponent>().localScale = {0.5f, 0.05f, 0.5f};
    root.AddComponent<WaypointMarkerComponent>();

    MeshRendererComponent mesh;
    mesh.meshAsset = 2462135637550940025ull;
    mesh.materialAsset = 616265134277951580ull;
    mesh.visible = true;
    mesh.castsShadows = false;
    mesh.receivesShadows = false;
    root.AddComponent<MeshRendererComponent>(mesh);

    const std::filesystem::path path = Paths::TempFile("waypoint_marker.prefab.yaml");
    HK_CHECK(static_cast<bool>(PrefabSerializer::Save(prefabScene, root, path)));
    return path;
}

Entity FindWaypointMarker(Scene& scene, uint32_t playerIndex) {
    auto view = scene.Registry().view<WaypointMarkerComponent>();
    for (const entt::entity handle : view) {
        Entity marker(handle, &scene);
        if (marker.GetComponent<WaypointMarkerComponent>().ownerPlayerIndex == playerIndex) {
            return marker;
        }
    }
    return {};
}

void PushWaypoint(GameplayWorld& world, uint32_t playerIndex, uint64_t tick, const glm::vec3& target) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.moveTarget = target;
    input.setMoveTarget = true;
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

} // namespace

void RunWaypointMarkerTests() {
    HockeyTest::BeginSuite("WaypointMarkerTests");

    RegisterGameplayComponents();

    Scene scene("WaypointMarkerScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.pregameCountdownSeconds = 0.0f;
    settings.waypointPrefabPath = SaveWaypointPrefab();

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    SetPlaying(scene);

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    HK_CHECK(skater.IsValid());
    const uint32_t playerIndex = skater.GetComponent<PlayerComponent>().playerIndex;

    PushWaypoint(world, playerIndex, 1, {3.0f, 0.0f, -5.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);

    Entity marker = FindWaypointMarker(scene, playerIndex);
    HK_CHECK_MSG(marker.IsValid(), "waypoint marker is created for move target");
    HK_CHECK(marker.HasComponent<PrefabComponent>());
    HK_CHECK(marker.HasComponent<MeshRendererComponent>());
    HK_CHECK(marker.GetComponent<ActiveComponent>().active);
    HK_CHECK_EQ(marker.GetComponent<WaypointMarkerComponent>().ownerPlayerIndex, playerIndex);
    HK_CHECK_NEAR(marker.GetComponent<TransformComponent>().localPosition.x, 3.0f, 0.001f);
    HK_CHECK_NEAR(marker.GetComponent<TransformComponent>().localPosition.z, -5.0f, 0.001f);

    const UUID markerId = marker.GetUUID();

    PushBrake(world, playerIndex, 2);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 2);
    marker = FindWaypointMarker(scene, playerIndex);
    HK_CHECK(marker.IsValid());
    HK_CHECK(!marker.GetComponent<ActiveComponent>().active);
    HK_CHECK(!marker.GetComponent<WaypointMarkerComponent>().active);

    PushWaypoint(world, playerIndex, 3, {-2.0f, 0.0f, -4.0f});
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 3);
    marker = FindWaypointMarker(scene, playerIndex);
    HK_CHECK(marker.IsValid());
    HK_CHECK_EQ(marker.GetUUID(), markerId);
    HK_CHECK(marker.GetComponent<ActiveComponent>().active);
    HK_CHECK_NEAR(marker.GetComponent<TransformComponent>().localPosition.x, -2.0f, 0.001f);
    HK_CHECK_NEAR(marker.GetComponent<TransformComponent>().localPosition.z, -4.0f, 0.001f);

    settings.waypointPrefabPath.clear();
    GameplayWorld disabledMarkerWorld;
    Scene disabledScene("WaypointMarkerDisabledScene");
    BuildValidGameplayScene(disabledScene);
    HK_CHECK(static_cast<bool>(disabledMarkerWorld.Init(disabledScene, nullptr, settings)));
    SetPlaying(disabledScene);
    Entity disabledSkater = FindPlayer(disabledScene, PlayerSlot::HomeSkater0);
    const uint32_t disabledIndex = disabledSkater.GetComponent<PlayerComponent>().playerIndex;
    PushWaypoint(disabledMarkerWorld, disabledIndex, 1, {1.0f, 0.0f, 1.0f});
    disabledMarkerWorld.FixedUpdate(disabledScene, settings.fixedDeltaSeconds, 1);
    HK_CHECK(!FindWaypointMarker(disabledScene, disabledIndex).IsValid());
}
