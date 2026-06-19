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
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Screenshot.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Gizmos/CameraLightGizmo.hpp"
#include "Hockey/Editor/Gizmos/GridGizmo.hpp"
#include "Hockey/Editor/Gizmos/PhysicsGizmo.hpp"
#include "Hockey/Editor/Gizmos/SelectionOutline.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Editor/SceneViewOverlay.hpp"
#include "Hockey/Editor/ViewportFrame.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
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

void QueueViewportScreenshot(Renderer& renderer, TextureHandle target, const char* prefix) {
    const std::filesystem::path path = MakeScreenshotPath(prefix);
    if (const Status status = renderer.RequestRenderTargetScreenshot(target, path); !status) {
        HK_EDITOR_WARN("Screenshot request failed: {}", status.error);
    } else {
        HK_EDITOR_INFO("Screenshot queued: {}", path.string());
    }
}

glm::mat4 GizmoProjectionFromRenderCamera(const CameraRenderData& camera) {
    glm::mat4 projection = camera.projection;
    projection[1][1] *= -1.0f; // remove the Vulkan image-space Y flip for ImGuizmo
    return projection;
}

void RayFromRenderCamera(const CameraRenderData& camera, const glm::vec2& viewportUV, glm::vec3& outOrigin,
                         glm::vec3& outDirection) {
    const float ndcX = viewportUV.x * 2.0f - 1.0f;
    const float ndcY = viewportUV.y * 2.0f - 1.0f; // render projection already contains the Vulkan Y flip
    const glm::mat4 invViewProj = glm::inverse(camera.projection * camera.view);

    glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farPoint = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    outOrigin = glm::vec3(nearPoint);
    outDirection = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
}

PhysicsGizmo::SubmitStats SubmitSceneDebugOverlay(Renderer& renderer, EditorContext& context, float viewportAspect) {
    DebugDraw& debug = renderer.Debug();
    if (context.settings.showGrid) {
        GridGizmo::Submit(debug, context.settings.gridSpacing);
    }
    CameraLightGizmo::Submit(debug, *context.activeScene, context.selection, viewportAspect);
    SelectionOutline::Submit(debug, *context.activeScene, context.selection);
    return PhysicsGizmo::Submit(debug, *context.activeScene, context.physicsView);
}

bool PhysicsVisualizationRequested(const PhysicsViewState& view) {
    return view.showColliders || view.showTriggers || view.showBodyCenters || view.showContacts;
}

bool ShouldShowEmptyPhysicsNotice(EditorContext& context, const PhysicsGizmo::SubmitStats& stats) {
    if (!PhysicsVisualizationRequested(context.physicsView) || stats.HasAny()) {
        return false;
    }
    return context.physicsView.visualizationOptionsOpen;
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

void SceneViewportPanel::EnsureCaptureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (!m_CaptureTarget.IsValid()) {
        RenderTargetDesc desc;
        desc.width = width;
        desc.height = height;
        desc.hasDepth = true;
        desc.debugName = "EditorSceneViewportCapture";
        m_CaptureTarget = context.renderer->CreateRenderTarget(desc);
        m_CaptureWidth = width;
        m_CaptureHeight = height;
        return;
    }
    if (width != m_CaptureWidth || height != m_CaptureHeight) {
        context.renderer->ResizeRenderTarget(m_CaptureTarget, width, height);
        m_CaptureWidth = width;
        m_CaptureHeight = height;
    }
}

void SceneViewportPanel::UpdateCamera(EditorContext& context, bool hovered) {
    const ImGuiIO& io = ImGui::GetIO();

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
    if (!hovered || io.WantTextInput || m_Navigating) {
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

void SceneViewportPanel::Pick(EditorContext& context, const CameraRenderData& camera, const glm::vec2& viewportUV,
                              const glm::vec2& viewportPixels, bool additive) {
    Scene& scene = *context.activeScene;
    const UUID handlePick = CameraLightGizmo::PickOrigin(scene, camera, viewportUV, viewportPixels);
    if (handlePick.IsValid()) {
        if (additive) {
            context.selection.Toggle(handlePick);
        } else {
            context.selection.Select(handlePick);
        }
        return;
    }

    glm::vec3 origin;
    glm::vec3 dir;
    RayFromRenderCamera(camera, viewportUV, origin, dir);

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
    if (renderer == nullptr || bridge == nullptr || context.activeScene == nullptr) {
        ImGui::TextUnformatted("Scene viewport unavailable.");
        return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 basePos = ImGui::GetCursorScreenPos();
    const EditorViewport::Frame frame = EditorViewport::ComputeFrame(basePos, available, renderer->GetSettings());
    const auto width = frame.width;
    const auto height = frame.height;

    if (width < 8 || height < 8) {
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

    const glm::vec2 imagePosGlm(frame.imagePos.x, frame.imagePos.y);
    const glm::vec2 imageSizeGlm(frame.imageSize.x, frame.imageSize.y);
    const glm::vec2 mouseGlm(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
    const bool controlsHovered = SceneViewOverlay::ContainsControls(imagePosGlm, imageSizeGlm, mouseGlm);
    const bool hovered =
        ImGui::IsWindowHovered() && EditorViewport::Contains(frame.imagePos, frame.imageSize, ImGui::GetIO().MousePos) &&
        !controlsHovered;
    const float aspect = frame.aspect;
    CameraRenderData gameCamera;
    const bool hasActiveCamera = FindActiveCamera(*context.activeScene, aspect, gameCamera);
    if (m_FollowGameCamera && hasActiveCamera) {
        m_Navigating = false;
    } else {
        UpdateCamera(context, hovered);
    }
    CameraRenderData sceneCamera =
        (m_FollowGameCamera && hasActiveCamera) ? gameCamera : context.editorCamera.RenderData(aspect);
    ProcessHotkeys(context, hovered);
    if (hovered && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
        QueueViewportScreenshot(*renderer, m_Target, "editor_scene");
    }

    if (context.captureSceneViewport) {
        EnsureCaptureTarget(context, context.captureViewportWidth, context.captureViewportHeight);
        if (m_CaptureTarget.IsValid()) {
            const float captureAspect =
                static_cast<float>(context.captureViewportWidth) / static_cast<float>(context.captureViewportHeight);
            CameraRenderData captureCamera;
            if (!(m_FollowGameCamera && FindActiveCamera(*context.activeScene, captureAspect, captureCamera))) {
                captureCamera = context.editorCamera.RenderData(captureAspect);
            }
            renderer->Debug().Clear();
            SubmitSceneDebugOverlay(*renderer, context, captureAspect);
            QueueViewportScreenshot(*renderer, m_CaptureTarget, (context.captureViewportPrefix + "_scene").c_str());
            renderer->RenderSceneToTarget(*context.activeScene, captureCamera, m_CaptureTarget);
        }
        context.captureSceneViewport = false;
    }

    // Submit overlay geometry before rendering so it is drawn into the target.
    renderer->Debug().Clear();
    const PhysicsGizmo::SubmitStats physicsStats = SubmitSceneDebugOverlay(*renderer, context, aspect);

    renderer->RenderSceneToTarget(*context.activeScene, sceneCamera, m_Target);

    const ImVec2 imagePos = frame.imagePos;
    const ImVec2 imageSize = frame.imageSize;
    const std::uint64_t textureId = bridge->ViewportTextureId(m_Target);
    ImGui::SetCursorScreenPos(imagePos);
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
                RayFromRenderCamera(sceneCamera, dropUV, rayOrigin, rayDir);
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

    const ImVec2 captureSize = ImGui::CalcTextSize("Capture");
    ImGui::SetCursorScreenPos(ImVec2(imagePos.x + imageSize.x - captureSize.x - 24.0f, imagePos.y + 110.0f));
    const bool captureClicked = ImGui::Button("Capture##scene_capture");
    const bool captureHovered = ImGui::IsItemHovered();
    if (captureClicked) {
        QueueViewportScreenshot(*renderer, m_Target, "editor_scene");
    }

    const glm::mat4 view = sceneCamera.view;
    const glm::mat4 gizmoProj = GizmoProjectionFromRenderCamera(sceneCamera);
    const bool gizmoActive = m_Gizmo.Manipulate(context, context.gizmoOperation, view, gizmoProj, imagePos.x,
                                                imagePos.y, imageSize.x, imageSize.y);
    SceneViewOverlayResult overlay =
        SceneViewOverlay::Draw(context, *context.activeScene, sceneCamera, imagePosGlm, imageSizeGlm, hasActiveCamera,
                               gameCamera, m_FollowGameCamera);
    if (!hasActiveCamera) {
        m_FollowGameCamera = false;
    }
    if (ShouldShowEmptyPhysicsNotice(context, physicsStats)) {
        ImGui::SetCursorScreenPos(ImVec2(imagePos.x + 8.0f, imagePos.y + 36.0f));
        ImGui::TextDisabled("Physics visualization: no collider/body/contact components in this scene");
    }
    const bool overlayBlocked = controlsHovered || captureHovered || captureClicked || overlay.hovered || overlay.capturedMouse;

    if (imageHovered && !overlayBlocked && !gizmoActive && !m_Navigating && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const glm::vec2 viewportUV((mouse.x - imagePos.x) / imageSize.x, (mouse.y - imagePos.y) / imageSize.y);
        Pick(context, sceneCamera, viewportUV, glm::vec2(imageSize.x, imageSize.y), ImGui::GetIO().KeyCtrl);
    }
}

} // namespace Hockey
