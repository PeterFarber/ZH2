#include "Hockey/Gameplay/Stick/StickAttachment.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

bool IsDirectChildOf(Entity child, Entity parent) {
    if (!child.IsValid() || !child.HasComponent<ParentComponent>()) {
        return false;
    }
    return child.GetComponent<ParentComponent>().parentId == parent.GetUUID();
}

Entity FindAttachedStickByPrefabSource(Scene& scene, Entity player, UUID sourceStickId) {
    for (Entity child : scene.GetChildren(player)) {
        if (!child.HasComponent<PrefabComponent>()) {
            continue;
        }
        const PrefabComponent& prefab = child.GetComponent<PrefabComponent>();
        if (prefab.sourceEntityId == sourceStickId) {
            return child;
        }
    }
    return {};
}

Entity ResolveAttachedStick(Scene& scene, Entity player, UUID stickEntityId) {
    if (!stickEntityId.IsValid()) {
        return {};
    }

    Entity stick = scene.FindEntityByUUID(stickEntityId);
    if (stick.IsValid() && IsDirectChildOf(stick, player)) {
        return stick;
    }

    return FindAttachedStickByPrefabSource(scene, player, stickEntityId);
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
        !player.GetComponent<StickAttachmentComponent>().stickEntityId.IsValid()) {
        ApplyStickGameplayData(player, {});
        return Status::Ok();
    }

    const UUID stickEntityId = player.GetComponent<StickAttachmentComponent>().stickEntityId;
    Entity stickEntity = ResolveAttachedStick(scene, player, stickEntityId);
    if (!stickEntity.IsValid()) {
        return Status::Fail("stick attachment reference '" + stickEntityId.ToString() +
                            "' did not resolve to a child stick entity");
    }
    if (!stickEntity.HasComponent<StickComponent>()) {
        return Status::Fail("stick attachment reference '" + stickEntityId.ToString() +
                            "' resolved to a child without StickComponent");
    }

    ApplyStickGameplayData(player, stickEntity);
    return Status::Ok();
}

} // namespace Hockey
