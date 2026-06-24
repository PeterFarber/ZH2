#include "Hockey/Editor/Gizmos/SelectionOutline.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Selection.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"

namespace Hockey::SelectionOutline {

namespace {
constexpr glm::vec4 kPrimaryColor{1.0f, 0.62f, 0.10f, 1.0f};
constexpr glm::vec4 kSecondaryColor{0.95f, 0.78f, 0.30f, 1.0f};
} // namespace

void Submit(DebugDraw& debug, Scene& scene, const Selection& selection) {
    const UUID primary = selection.Primary();
    for (const UUID id : selection.All()) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || !entity.HasComponent<TransformComponent>()) {
            continue;
        }
        const glm::mat4 world = scene.GetWorldTransform(entity);
        const glm::vec4 color = (id == primary) ? kPrimaryColor : kSecondaryColor;
        debug.DrawBox(world, color);
    }
}

void Submit(DebugDraw& debug, EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }

    Scene& scene = *context.activeScene;
    const UUID primary = context.selection.Primary();
    for (const UUID id : context.selection.All()) {
        if (context.IsSceneHidden(id)) {
            continue;
        }
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || !entity.HasComponent<TransformComponent>()) {
            continue;
        }
        const glm::mat4 world = scene.GetWorldTransform(entity);
        const glm::vec4 color = (id == primary) ? kPrimaryColor : kSecondaryColor;
        debug.DrawBox(world, color);
    }
}

} // namespace Hockey::SelectionOutline
