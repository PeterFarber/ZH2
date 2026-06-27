#include "Hockey/Gameplay/Stick/StickAttachment.hpp"

#include <cstddef>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

namespace Hockey {
namespace {

struct StickChildSearch {
    Entity stick;
    std::size_t candidateCount = 0;
    bool ambiguous = false;
};

StickChildSearch FindDirectStickChild(Scene& scene, Entity player) {
    StickChildSearch result;
    Entity namedStick;
    std::size_t namedStickCount = 0;

    for (Entity child : scene.GetChildren(player)) {
        if (!child.HasComponent<StickComponent>()) {
            continue;
        }

        ++result.candidateCount;
        if (!result.stick.IsValid()) {
            result.stick = child;
        }
        if (child.GetName() == "Stick") {
            ++namedStickCount;
            if (!namedStick.IsValid()) {
                namedStick = child;
            }
        }
    }

    if (namedStickCount == 1) {
        result.stick = namedStick;
        result.ambiguous = false;
        return result;
    }

    result.ambiguous = result.candidateCount > 1;
    if (namedStickCount > 1) {
        result.ambiguous = true;
    }
    return result;
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

    const StickChildSearch childStick = FindDirectStickChild(scene, player);
    if (childStick.ambiguous) {
        return Status::Fail("player '" + player.GetName() +
                            "' has multiple direct stick children; keep one StickComponent child or name exactly one child 'Stick'");
    }

    if (!childStick.stick.IsValid()) {
        ApplyStickGameplayData(player, {});
        return Status::Ok();
    }

    ApplyStickGameplayData(player, childStick.stick);
    return Status::Ok();
}

} // namespace Hockey
