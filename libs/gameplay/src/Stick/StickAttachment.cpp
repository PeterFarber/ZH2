#include "Hockey/Gameplay/Stick/StickAttachment.hpp"

#include <vector>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {
namespace {

std::vector<Entity> FindDirectStickChildren(Scene& scene, Entity player) {
    std::vector<Entity> sticks;
    for (Entity child : scene.GetChildren(player)) {
        if (!child.HasComponent<StickComponent>()) {
            continue;
        }
        sticks.push_back(child);
    }
    return sticks;
}

void DestroyDirectStickChildren(Scene& scene, Entity player) {
    const std::vector<Entity> existingSticks = FindDirectStickChildren(scene, player);
    for (Entity stick : existingSticks) {
        scene.DestroyEntityRecursive(stick);
    }
}

Result<Entity> CreateStickEntity(Scene& scene, Entity player, const StickTuning& tuning) {
    Entity stick;
    if (!tuning.prefabPath.empty()) {
        Result<Entity> instantiated = PrefabSerializer::Instantiate(scene, tuning.prefabPath);
        if (!instantiated) {
            return Result<Entity>::Fail("failed to instantiate stick prefab '" + tuning.prefabPath.generic_string() +
                                        "': " + instantiated.error);
        }
        stick = instantiated.value;
    } else {
        stick = scene.CreateEntity("Stick");
    }

    stick.GetComponent<NameComponent>().name = "Stick";
    scene.SetParent(stick, player, false);
    return Result<Entity>::Ok(stick);
}

void ApplyStickGameplayData(Entity player, Entity stickEntity, const StickTuning& tuning) {
    StickComponent stick;
    stick.ownerPlayer = player.GetUUID();
    stick.reach = tuning.reach;
    stick.width = tuning.width;
    stick.localOffset = tuning.localOffset;
    stick.canControlPuck = true;
    stick.canShoot = true;
    player.AddOrReplaceComponent<StickComponent>(stick);

    if (stickEntity.IsValid()) {
        stickEntity.AddOrReplaceComponent<StickComponent>(stick);
    }
}

} // namespace

Status StickAttachment::EnsureStickAttached(Scene& scene, Entity player, const StickTuning& tuning) {
    if (!player.IsValid()) {
        return Status::Fail("stick attachment requires a valid player entity");
    }

    DestroyDirectStickChildren(scene, player);
    Result<Entity> stick = CreateStickEntity(scene, player, tuning);
    if (!stick) {
        return Status::Fail(stick.error);
    }
    ApplyStickGameplayData(player, stick.value, tuning);
    return Status::Ok();
}

} // namespace Hockey
