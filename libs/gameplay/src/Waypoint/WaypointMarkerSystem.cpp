#include "Hockey/Gameplay/Waypoint/WaypointMarkerSystem.hpp"

#include <filesystem>
#include <string>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Waypoint/WaypointMarkerComponents.hpp"

namespace Hockey {
namespace {

Entity FindMarkerForPlayer(Scene& scene, uint32_t playerIndex) {
    auto markers = scene.Registry().view<WaypointMarkerComponent>();
    for (const entt::entity handle : markers) {
        Entity marker(handle, &scene);
        if (marker.GetComponent<WaypointMarkerComponent>().ownerPlayerIndex == playerIndex) {
            return marker;
        }
    }
    return {};
}

Entity CreateMarker(Scene& scene, const std::filesystem::path& prefabPath, uint32_t playerIndex) {
    Entity marker;
    if (!prefabPath.empty()) {
        Result<Entity> prefab = PrefabSerializer::Instantiate(scene, prefabPath);
        if (prefab) {
            marker = prefab.value;
        }
    }

    if (!marker.IsValid()) {
        marker = scene.CreateEntity("Waypoint Marker");
    }

    marker.GetComponent<NameComponent>().name = "Waypoint Marker " + std::to_string(playerIndex);
    WaypointMarkerComponent component;
    if (marker.HasComponent<WaypointMarkerComponent>()) {
        component = marker.GetComponent<WaypointMarkerComponent>();
    }
    component.ownerPlayerIndex = playerIndex;
    marker.AddOrReplaceComponent<WaypointMarkerComponent>(component);
    return marker;
}

void SetMarkerState(Entity marker, uint32_t playerIndex, const glm::vec3& targetPosition, bool active) {
    if (!marker.IsValid()) {
        return;
    }

    WaypointMarkerComponent component;
    if (marker.HasComponent<WaypointMarkerComponent>()) {
        component = marker.GetComponent<WaypointMarkerComponent>();
    }
    component.ownerPlayerIndex = playerIndex;
    component.targetPosition = targetPosition;
    component.active = active;
    marker.AddOrReplaceComponent<WaypointMarkerComponent>(component);

    marker.GetComponent<TransformComponent>().localPosition = targetPosition;
    marker.SetActive(active);
}

void DeactivateAllMarkers(Scene& scene) {
    auto markers = scene.Registry().view<WaypointMarkerComponent>();
    for (const entt::entity handle : markers) {
        Entity marker(handle, &scene);
        marker.GetComponent<WaypointMarkerComponent>().active = false;
        marker.SetActive(false);
    }
}

} // namespace

void WaypointMarkerSystem::FixedUpdate(Scene& scene, const GameplaySettings& settings) {
    if (settings.waypointPrefabPath.empty()) {
        DeactivateAllMarkers(scene);
        return;
    }

    auto players = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();
    for (const entt::entity handle : players) {
        Entity player(handle, &scene);
        const PlayerComponent& playerComponent = player.GetComponent<PlayerComponent>();
        const PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();

        Entity marker = FindMarkerForPlayer(scene, playerComponent.playerIndex);
        if (!runtime.hasMoveTarget) {
            if (marker.IsValid()) {
                SetMarkerState(marker, playerComponent.playerIndex, runtime.moveTarget, false);
            }
            continue;
        }

        if (!marker.IsValid()) {
            marker = CreateMarker(scene, settings.waypointPrefabPath, playerComponent.playerIndex);
        }
        SetMarkerState(marker, playerComponent.playerIndex, runtime.moveTarget, true);
    }
}

} // namespace Hockey
