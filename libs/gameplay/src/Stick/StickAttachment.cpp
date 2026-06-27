#include "Hockey/Gameplay/Stick/StickAttachment.hpp"

#include <filesystem>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

Entity FindExistingAttachedStick(Scene& scene, Entity player, const std::filesystem::path& prefabPath) {
    for (Entity child : scene.GetChildren(player)) {
        if (!child.HasComponent<PrefabComponent>()) {
            continue;
        }
        const PrefabComponent& prefab = child.GetComponent<PrefabComponent>();
        if (prefab.sourcePath == prefabPath) {
            return child;
        }
    }
    return {};
}

void ApplyStickGameplayData(Entity player, Entity stickEntity) {
    StickComponent stick;
    if (player.HasComponent<StickComponent>()) {
        stick = player.GetComponent<StickComponent>();
    }
    if (stickEntity.IsValid() && stickEntity.HasComponent<StickComponent>()) {
        stick = stickEntity.GetComponent<StickComponent>();
    }
    stick.ownerPlayer = player.GetUUID();
    player.AddOrReplaceComponent<StickComponent>(stick);

    if (stickEntity.IsValid() && stickEntity.HasComponent<StickComponent>()) {
        StickComponent authored = stickEntity.GetComponent<StickComponent>();
        authored.ownerPlayer = player.GetUUID();
        stickEntity.AddOrReplaceComponent<StickComponent>(authored);
    }
}

} // namespace

Status StickAttachment::EnsureStickAttached(Scene& scene, Entity player) {
    if (!player.IsValid()) {
        return Status::Fail("stick attachment requires a valid player entity");
    }

    if (!player.HasComponent<StickAttachmentComponent>() ||
        player.GetComponent<StickAttachmentComponent>().stickPrefabPath.empty()) {
        ApplyStickGameplayData(player, {});
        return Status::Ok();
    }

    const std::filesystem::path prefabPath = player.GetComponent<StickAttachmentComponent>().stickPrefabPath;
    Entity stickEntity = FindExistingAttachedStick(scene, player, prefabPath);
    if (!stickEntity.IsValid()) {
        Result<Entity> instantiated = PrefabSerializer::Instantiate(scene, prefabPath);
        if (!instantiated) {
            return Status::Fail("failed to instantiate stick prefab '" + prefabPath.generic_string() +
                                "': " + instantiated.error);
        }
        stickEntity = instantiated.value;
        stickEntity.GetComponent<NameComponent>().name = "Stick";
        scene.SetParent(stickEntity, player, false);
    }

    ApplyStickGameplayData(player, stickEntity);
    return Status::Ok();
}

} // namespace Hockey
