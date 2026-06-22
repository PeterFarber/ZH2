#include "Hockey/Editor/EditorTransformOperations.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Selection.hpp"

namespace Hockey::EditorTransformOperations {

namespace {

TransformData LocalOf(Entity entity) {
    const TransformComponent& transform = entity.GetComponent<TransformComponent>();
    return {transform.localPosition, transform.localRotation, transform.localScale};
}

bool Differs(const TransformData& a, const TransformData& b) {
    return a.position != b.position || a.rotation != b.rotation || a.scale != b.scale;
}

} // namespace

std::vector<UUID> TopLevelTransformableSelection(Scene& scene, const Selection& selection) {
    std::vector<UUID> result;
    for (const UUID id : selection.All()) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || !entity.HasComponent<TransformComponent>()) {
            continue;
        }

        bool ancestorSelected = false;
        for (Entity parent = scene.GetParent(entity); parent; parent = scene.GetParent(parent)) {
            if (selection.IsSelected(parent.GetUUID())) {
                ancestorSelected = true;
                break;
            }
        }

        if (!ancestorSelected) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<EntityTransformSnapshot> CaptureLocalSnapshots(Scene& scene, const std::vector<UUID>& entityIds) {
    std::vector<EntityTransformSnapshot> snapshots;
    snapshots.reserve(entityIds.size());
    for (const UUID id : entityIds) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || !entity.HasComponent<TransformComponent>()) {
            continue;
        }
        const TransformData local = LocalOf(entity);
        snapshots.push_back({id, local, local});
    }
    return snapshots;
}

void CaptureCurrentAsAfter(Scene& scene, std::vector<EntityTransformSnapshot>& snapshots) {
    for (EntityTransformSnapshot& snapshot : snapshots) {
        Entity entity = scene.FindEntityByUUID(snapshot.entityId);
        if (entity && entity.HasComponent<TransformComponent>()) {
            snapshot.after = LocalOf(entity);
        }
    }
}

std::vector<EntityTransformSnapshot> ChangedSnapshots(const std::vector<EntityTransformSnapshot>& snapshots) {
    std::vector<EntityTransformSnapshot> changed;
    for (const EntityTransformSnapshot& snapshot : snapshots) {
        if (Differs(snapshot.before, snapshot.after)) {
            changed.push_back(snapshot);
        }
    }
    return changed;
}

void ApplyWorldTranslationDelta(Scene& scene, const std::vector<UUID>& entityIds, const glm::vec3& delta) {
    if (delta == glm::vec3(0.0f)) {
        return;
    }

    for (const UUID id : entityIds) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || !entity.HasComponent<TransformComponent>()) {
            continue;
        }

        glm::mat4 world = scene.GetWorldTransform(entity);
        world[3] = glm::vec4(glm::vec3(world[3]) + delta, world[3].w);
        scene.SetWorldTransform(entity, world);
    }
}

} // namespace Hockey::EditorTransformOperations
