#include "Hockey/Editor/Panels/SceneViewportPanel.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <utility>

#include <glm/glm.hpp>

#include <imgui.h>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Gizmos/GridGizmo.hpp"
#include "Hockey/Editor/Gizmos/PhysicsGizmo.hpp"
#include "Hockey/Editor/Gizmos/SelectionOutline.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

namespace {

// Ray vs. the unit cube [-0.5, 0.5]^3 in the ray's coordinate space. Returns the
// nearest non-negative hit distance (origin-inside yields the exit distance).
bool RayUnitBox(const glm::vec3& origin, const glm::vec3& dir, float& outDistance) {
    constexpr glm::vec3 boxMin{-0.5f, -0.5f, -0.5f};
    constexpr glm::vec3 boxMax{0.5f, 0.5f, 0.5f};
    float tMin = -std::numeric_limits<float>::max();
    float tMax = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(dir[axis]) < 1e-8f) {
            if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis]) {
                return false;
            }
            continue;
        }
        const float inv = 1.0f / dir[axis];
        float t1 = (boxMin[axis] - origin[axis]) * inv;
        float t2 = (boxMax[axis] - origin[axis]) * inv;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) {
            return false;
        }
    }
    outDistance = (tMin >= 0.0f) ? tMin : tMax;
    return outDistance >= 0.0f;
}

} // namespace

SceneViewportPanel::SceneViewportPanel() : Panel(EditorPanelNames::kSceneViewport) {}

void SceneViewportPanel::OnImGui(EditorContext& context) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = BeginWindow();
    ImGui::PopStyleVar();
    if (visible) {
        DrawViewport(context);
    }
    EndWindow();
}

void SceneViewportPanel::EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height) {
    if (!m_Target.IsValid()) {
        RenderTargetDesc desc;
        desc.width = width;
        desc.height = height;
        desc.hasDepth = true;
        desc.debugName = "EditorSceneViewport";
        m_Target = context.renderer->CreateRenderTarget(desc);
        m_Width = width;
        m_Height = height;
        return;
    }
    if (width != m_Width || height != m_Height) {
        context.renderer->ResizeRenderTarget(m_Target, width, height);
        if (context.imguiBridge != nullptr) {
            context.imguiBridge->InvalidateViewportTextures();
        }
        m_Width = width;
        m_Height = height;
    }
}

void SceneViewportPanel::UpdateCamera(EditorContext& context, bool hovered) {
    const ImGuiIO& io = ImGui::GetIO();

    if (context.gameplayView.previewEnabled) {
        m_Navigating = false;
        context.editorCamera.Update({}, io.DeltaTime, context.settings);
        return;
    }

    const bool look = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool pan = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    const bool orbit = io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool anyNav = look || pan || orbit;

    if (!m_Navigating && hovered && anyNav) {
        m_Navigating = true;
    }
    if (!anyNav) {
        m_Navigating = false;
    }
    const bool active = m_Navigating;

    EditorCamera::Input input;
    input.hovered = hovered;
    input.mouseDelta = {io.MouseDelta.x, io.MouseDelta.y};
    input.wheel = io.MouseWheel;
    input.look = active && look;
    input.pan = active && pan && !look;
    input.orbit = active && orbit && !look && !pan;
    if (input.look) {
        input.forward = ImGui::IsKeyDown(ImGuiKey_W);
        input.back = ImGui::IsKeyDown(ImGuiKey_S);
        input.left = ImGui::IsKeyDown(ImGuiKey_A);
        input.right = ImGui::IsKeyDown(ImGuiKey_D);
        input.up = ImGui::IsKeyDown(ImGuiKey_E);
        input.down = ImGui::IsKeyDown(ImGuiKey_Q);
        input.fast = io.KeyShift;
    }
    context.editorCamera.Update(input, io.DeltaTime, context.settings);
}

void SceneViewportPanel::ProcessHotkeys(EditorContext& context, bool hovered) {
    const ImGuiIO& io = ImGui::GetIO();
    if (!hovered || io.WantTextInput || m_Navigating || context.gameplayView.previewEnabled) {
        return;
    }
    // Transform-tool shortcuts route through the ToolManager so the toolbar/menu
    // highlight and the gizmo mode stay in sync.
    if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
        context.toolManager.Activate("Select", context);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        context.toolManager.Activate("Move", context);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        context.toolManager.Activate("Rotate", context);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        context.toolManager.Activate("Scale", context);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        Entity entity = context.activeScene->FindEntityByUUID(context.selection.Primary());
        if (entity && entity.HasComponent<TransformComponent>()) {
            const glm::mat4 world = context.activeScene->GetWorldTransform(entity);
            const glm::vec3 position{world[3]};
            const float sx = glm::length(glm::vec3(world[0]));
            const float sy = glm::length(glm::vec3(world[1]));
            const float sz = glm::length(glm::vec3(world[2]));
            const float radius = std::max({sx, sy, sz, 1.0f}) * 0.75f;
            context.editorCamera.Focus(position, radius);
        }
    }
}

void SceneViewportPanel::Pick(EditorContext& context, const glm::vec2& viewportUV, float aspect, bool additive) {
    Scene& scene = *context.activeScene;
    glm::vec3 origin;
    glm::vec3 dir;
    context.editorCamera.Ray(viewportUV, aspect, origin, dir);

    UUID best{0};
    float bestDistance = std::numeric_limits<float>::max();
    for (Entity entity : scene.GetAllEntities()) {
        if (!entity.HasComponent<TransformComponent>()) {
            continue;
        }
        const glm::mat4 world = scene.GetWorldTransform(entity);
        const glm::mat4 invWorld = glm::inverse(world);
        const glm::vec3 localOrigin{invWorld * glm::vec4(origin, 1.0f)};
        const glm::vec3 localDir{invWorld * glm::vec4(dir, 0.0f)};
        float distance = 0.0f;
        if (RayUnitBox(localOrigin, localDir, distance) && distance < bestDistance) {
            bestDistance = distance;
            best = entity.GetUUID();
        }
    }

    if (best.IsValid()) {
        if (additive) {
            context.selection.Toggle(best);
        } else {
            context.selection.Select(best);
        }
    } else if (!additive) {
        context.selection.Clear();
    }
}

void SceneViewportPanel::DrawViewport(EditorContext& context) {
    Renderer* renderer = context.renderer;
    ImGuiRendererBridge* bridge = context.imguiBridge;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const auto width = static_cast<std::uint32_t>(std::max(1.0f, available.x));
    const auto height = static_cast<std::uint32_t>(std::max(1.0f, available.y));

    if (renderer == nullptr || bridge == nullptr || context.activeScene == nullptr || width < 8 || height < 8) {
        ImGui::TextUnformatted("Scene viewport unavailable.");
        return;
    }

    EnsureTarget(context, width, height);
    if (!m_Target.IsValid()) {
        ImGui::TextUnformatted("Failed to create the viewport render target.");
        return;
    }
    context.viewportWidth = m_Width;
    context.viewportHeight = m_Height;

    const bool hovered = ImGui::IsWindowHovered();
    UpdateCamera(context, hovered);
    ProcessHotkeys(context, hovered);

    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    // Submit overlay geometry before rendering so it is drawn into the target.
    if (context.settings.showGrid) {
        GridGizmo::Submit(renderer->Debug(), context.settings.gridSpacing);
    }
    SelectionOutline::Submit(renderer->Debug(), *context.activeScene, context.selection);
    PhysicsGizmo::Submit(renderer->Debug(), *context.activeScene, context.physicsView);

    renderer->RenderSceneToTarget(*context.activeScene, context.editorCamera.RenderData(aspect), m_Target);

    const ImVec2 imagePos = ImGui::GetCursorScreenPos();
    const ImVec2 imageSize(static_cast<float>(width), static_cast<float>(height));
    const std::uint64_t textureId = bridge->ViewportTextureId(m_Target);
    if (textureId != 0) {
        ImGui::Image(static_cast<ImTextureID>(textureId), imageSize);
    } else {
        ImGui::Dummy(imageSize);
    }
    const bool imageHovered = ImGui::IsItemHovered();

    // Dropping a prefab onto the viewport instantiates it where the cursor ray
    // meets the ground plane (y = 0), falling back to the origin when the ray is
    // parallel to or points away from the plane.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropType)) {
            const std::filesystem::path prefabPath(static_cast<const char*>(payload->Data));
            if (context.activeScene != nullptr) {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                const glm::vec2 dropUV((mouse.x - imagePos.x) / imageSize.x, (mouse.y - imagePos.y) / imageSize.y);
                glm::vec3 rayOrigin;
                glm::vec3 rayDir;
                context.editorCamera.Ray(dropUV, aspect, rayOrigin, rayDir);
                glm::vec3 dropPoint(0.0f);
                if (std::abs(rayDir.y) > 1e-5f) {
                    const float t = -rayOrigin.y / rayDir.y;
                    if (t > 0.0f) {
                        dropPoint = rayOrigin + t * rayDir;
                    }
                }
                context.undoRedo.Execute(EditorCommands::InstantiatePrefabAt(prefabPath, dropPoint), context);
            }
        }
        ImGui::EndDragDropTarget();
    }

    const glm::mat4 view = context.editorCamera.ViewMatrix();
    const glm::mat4 gizmoProj = context.editorCamera.GizmoProjection(aspect);
    const bool gizmoActive = m_Gizmo.Manipulate(context, context.gizmoOperation, view, gizmoProj, imagePos.x,
                                                imagePos.y, imageSize.x, imageSize.y);

    if (imageHovered && !gizmoActive && !m_Navigating && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const glm::vec2 viewportUV((mouse.x - imagePos.x) / imageSize.x, (mouse.y - imagePos.y) / imageSize.y);
        Pick(context, viewportUV, aspect, ImGui::GetIO().KeyCtrl);
    }
}

} // namespace Hockey
