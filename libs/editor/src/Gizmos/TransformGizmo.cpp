#include "Hockey/Editor/Gizmos/TransformGizmo.hpp"

#include <array>
#include <utility>

#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <ImGuizmo.h>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorTransformOperations.hpp"

namespace Hockey {

namespace {

ImGuizmo::OPERATION ToImGuizmo(GizmoOperation operation) {
    switch (operation) {
    case GizmoOperation::Rotate:
        return ImGuizmo::ROTATE;
    case GizmoOperation::Scale:
        return ImGuizmo::SCALE;
    case GizmoOperation::Translate:
    case GizmoOperation::None:
    default:
        return ImGuizmo::TRANSLATE;
    }
}

float SnapValue(GizmoOperation operation, const EditorSettings& settings) {
    switch (operation) {
    case GizmoOperation::Rotate:
        return settings.rotateSnapDegrees;
    case GizmoOperation::Scale:
        return settings.scaleSnap;
    case GizmoOperation::Translate:
    case GizmoOperation::None:
    default:
        return settings.moveSnap;
    }
}

TransformData LocalOf(Entity entity) {
    const TransformComponent& transform = entity.GetComponent<TransformComponent>();
    return {transform.localPosition, transform.localRotation, transform.localScale};
}

bool Differs(const TransformData& a, const TransformData& b) {
    return a.position != b.position || a.rotation != b.rotation || a.scale != b.scale;
}

} // namespace

bool TransformGizmo::Manipulate(EditorContext& context, GizmoOperation operation, const glm::mat4& view,
                                const glm::mat4& projection, float rectX, float rectY, float rectWidth,
                                float rectHeight) {
    if (operation == GizmoOperation::None || context.activeScene == nullptr) {
        m_Using = false;
        return false;
    }

    Scene& scene = *context.activeScene;
    const UUID primary = context.selection.Primary();
    Entity entity = scene.FindEntityByUUID(primary);
    if (!entity || !entity.HasComponent<TransformComponent>()) {
        m_Using = false;
        return false;
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(rectX, rectY, rectWidth, rectHeight);

    const ImGuizmo::OPERATION op = ToImGuizmo(operation);
    const ImGuizmo::MODE mode = (context.transformSpace == TransformSpace::Local) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    glm::mat4 world = scene.GetWorldTransform(entity);
    const glm::mat4 previousPrimaryWorld = world;

    const bool useSnap = context.settings.snapEnabled;
    const float snapScalar = SnapValue(operation, context.settings);
    const std::array<float, 3> snap{snapScalar, snapScalar, snapScalar};

    const bool changed = ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), op, mode,
                                              glm::value_ptr(world), nullptr, useSnap ? snap.data() : nullptr);
    const bool usingNow = ImGuizmo::IsUsing();

    if (usingNow && !m_Using) {
        m_Entity = primary;
        m_BeforeLocal = LocalOf(entity);
        m_GroupTranslate = operation == GizmoOperation::Translate && context.selection.Count() > 1;
        m_GroupEntities.clear();
        m_GroupSnapshots.clear();
        m_PreviousPrimaryWorld = previousPrimaryWorld;
        if (m_GroupTranslate) {
            m_GroupEntities = EditorTransformOperations::TopLevelTransformableSelection(scene, context.selection);
            m_GroupSnapshots = EditorTransformOperations::CaptureLocalSnapshots(scene, m_GroupEntities);
            if (m_GroupSnapshots.empty()) {
                m_GroupTranslate = false;
            }
        }
    }

    if (changed) {
        if (m_GroupTranslate) {
            const glm::vec3 delta = glm::vec3(world[3]) - glm::vec3(m_PreviousPrimaryWorld[3]);
            EditorTransformOperations::ApplyWorldTranslationDelta(scene, m_GroupEntities, delta);
            if (Entity edited = scene.FindEntityByUUID(m_Entity)) {
                m_PreviousPrimaryWorld = scene.GetWorldTransform(edited);
            } else {
                m_PreviousPrimaryWorld = world;
            }
        } else {
            scene.SetWorldTransform(entity, world);
        }
    }

    if (!usingNow && m_Using) {
        if (m_GroupTranslate) {
            EditorTransformOperations::CaptureCurrentAsAfter(scene, m_GroupSnapshots);
            std::vector<EntityTransformSnapshot> changedSnapshots =
                EditorTransformOperations::ChangedSnapshots(m_GroupSnapshots);
            if (!changedSnapshots.empty()) {
                context.undoRedo.Execute(EditorCommands::TransformEntities(std::move(changedSnapshots)), context);
            }
        } else {
            Entity edited = scene.FindEntityByUUID(m_Entity);
            if (edited && edited.HasComponent<TransformComponent>()) {
                const TransformData after = LocalOf(edited);
                if (Differs(m_BeforeLocal, after)) {
                    context.undoRedo.Execute(EditorCommands::TransformEntity(m_Entity, m_BeforeLocal, after), context);
                }
            }
        }
    }

    m_Using = usingNow;
    if (!m_Using) {
        m_GroupTranslate = false;
        m_GroupEntities.clear();
        m_GroupSnapshots.clear();
    }
    return usingNow || ImGuizmo::IsOver();
}

} // namespace Hockey
