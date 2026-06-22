#include "Hockey/Editor/Panels/GameViewportPanel.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>

#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Mouse.hpp"
#include "Hockey/Core/Screenshot.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/ViewportFrame.hpp"
#include "Hockey/Renderer/Camera.hpp"
#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

namespace {
// Makes 'cameraId' the scene's only primary camera (clearing the flag on every
// other camera). Returns true if a camera flag actually changed.
bool SetPrimaryCamera(Scene& scene, UUID cameraId) {
    Entity target = scene.FindEntityByUUID(cameraId);
    if (!target || !target.HasComponent<CameraComponent>()) {
        return false;
    }
    for (const entt::entity handle : scene.Registry().view<CameraComponent>()) {
        scene.Registry().get<CameraComponent>(handle).primary = false;
    }
    target.GetComponent<CameraComponent>().primary = true;
    return true;
}

void QueueViewportScreenshot(Renderer& renderer, TextureHandle target, const char* prefix) {
    const std::filesystem::path path = MakeScreenshotPath(prefix);
    if (const Status status = renderer.RequestRenderTargetScreenshot(target, path); !status) {
        HK_EDITOR_WARN("Screenshot request failed: {}", status.error);
    } else {
        HK_EDITOR_INFO("Screenshot queued: {}", path.string());
    }
}

bool ProjectViewportMouseToIcePlane(const CameraRenderData& camera, const ImVec2& imagePos, const ImVec2& imageSize,
                                    glm::vec3& outPoint) {
    if (imageSize.x <= 0.0f || imageSize.y <= 0.0f) {
        return false;
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float u = (mouse.x - imagePos.x) / imageSize.x;
    const float v = (mouse.y - imagePos.y) / imageSize.y;
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
        return false;
    }

    const float ndcX = u * 2.0f - 1.0f;
    const float ndcY = v * 2.0f - 1.0f; // render projection already contains the Vulkan Y flip
    const glm::mat4 invViewProj = glm::inverse(camera.projection * camera.view);

    glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farPoint = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    if (std::abs(nearPoint.w) < 1e-6f || std::abs(farPoint.w) < 1e-6f) {
        return false;
    }
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    const glm::vec3 origin = glm::vec3(nearPoint);
    const glm::vec3 delta = glm::vec3(farPoint) - origin;
    if (glm::dot(delta, delta) < 1e-10f) {
        return false;
    }
    const glm::vec3 direction = glm::normalize(delta);
    if (std::abs(direction.y) < 1e-5f) {
        return false;
    }
    const float t = -origin.y / direction.y;
    if (t < 0.0f) {
        return false;
    }
    outPoint = origin + direction * t;
    return true;
}

} // namespace

GameViewportPanel::GameViewportPanel() : Panel(EditorPanelNames::kGameViewport) {}

void GameViewportPanel::OnImGui(EditorContext& context) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = BeginWindow();
    ImGui::PopStyleVar();
    if (visible) {
        DrawViewport(context);
    } else if (context.playMode) {
        context.gameplayView.gameInputActive = false;
    }
    EndWindow();
}

void GameViewportPanel::EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height) {
    if (!m_Target.IsValid()) {
        RenderTargetDesc desc;
        desc.width = width;
        desc.height = height;
        desc.hasDepth = true;
        desc.debugName = "EditorGameViewport";
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

void GameViewportPanel::EnsureCaptureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    if (!m_CaptureTarget.IsValid()) {
        RenderTargetDesc desc;
        desc.width = width;
        desc.height = height;
        desc.hasDepth = true;
        desc.debugName = "EditorGameViewportCapture";
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

void GameViewportPanel::DrawViewport(EditorContext& context) {
    Renderer* renderer = context.renderer;
    ImGuiRendererBridge* bridge = context.imguiBridge;
    if (renderer == nullptr || bridge == nullptr || context.activeScene == nullptr) {
        context.gameplayView.gameInputActive = false;
        ImGui::TextUnformatted("Game viewport unavailable.");
        return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 basePos = ImGui::GetCursorScreenPos();
    const EditorViewport::Frame frame = EditorViewport::ComputeFrame(basePos, available, renderer->GetSettings());
    const auto width = frame.width;
    const auto height = frame.height;

    if (width < 8 || height < 8) {
        context.gameplayView.gameInputActive = false;
        ImGui::TextUnformatted("Game viewport unavailable.");
        return;
    }

    Scene& scene = *context.activeScene;
    const ImVec2 imagePos = frame.imagePos;
    const ImVec2 imageSize = frame.imageSize;
    const float aspect = frame.aspect;

    CameraRenderData camera;
    const bool hasCamera = FindActiveCamera(scene, aspect, camera);
    context.gameplayView.gameInputActive = false;

    if (hasCamera) {
        EnsureTarget(context, width, height);
        if (ImGui::IsWindowHovered() && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
            QueueViewportScreenshot(*renderer, m_Target, "editor_game");
        }
        // The game preview is a clean camera view: drop any editor debug lines
        // (grid/selection) submitted by the scene viewport so they don't leak in.
        renderer->Debug().Clear();
        if (context.captureGameViewport) {
            EnsureCaptureTarget(context, context.captureViewportWidth, context.captureViewportHeight);
            if (m_CaptureTarget.IsValid()) {
                CameraRenderData captureCamera;
                const float captureAspect =
                    static_cast<float>(context.captureViewportWidth) / static_cast<float>(context.captureViewportHeight);
                if (FindActiveCamera(scene, captureAspect, captureCamera)) {
                    QueueViewportScreenshot(*renderer, m_CaptureTarget,
                                            (context.captureViewportPrefix + "_game").c_str());
                    renderer->RenderSceneToTarget(scene, captureCamera, m_CaptureTarget);
                }
            }
            context.captureGameViewport = false;
        }
        renderer->RenderSceneToTarget(scene, camera, m_Target);

        const std::uint64_t textureId = m_Target.IsValid() ? bridge->ViewportTextureId(m_Target) : 0;
        ImGui::SetCursorScreenPos(imagePos);
        if (textureId != 0) {
            ImGui::Image(static_cast<ImTextureID>(textureId), imageSize);
        } else {
            ImGui::Dummy(imageSize);
        }
        const bool imageHovered = ImGui::IsItemHovered();
        const bool gameViewActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || imageHovered;
        context.gameplayView.gameInputActive =
            context.playMode && hasCamera && gameViewActive && !ImGui::GetIO().WantTextInput;
        if (context.gameplayView.gameInputActive && imageHovered && context.gameplayPreview != nullptr) {
            glm::vec3 target{0.0f};
            if (ProjectViewportMouseToIcePlane(camera, imagePos, imageSize, target)) {
                context.gameplayPreview->SetAimTarget(target);
                if (Input::WasMouseButtonPressed(MouseButton::Right)) {
                    context.gameplayPreview->SetMoveTarget(target);
                }
            }
        }
        const ImVec2 captureSize = ImGui::CalcTextSize("Capture");
        ImGui::SetCursorScreenPos(ImVec2(imagePos.x + imageSize.x - captureSize.x - 24.0f, imagePos.y + 8.0f));
        if (ImGui::Button("Capture##game_capture")) {
            QueueViewportScreenshot(*renderer, m_Target, "editor_game");
        }
    } else {
        ImGui::SetCursorScreenPos(imagePos);
        ImGui::Dummy(imageSize);
        const char* message = "No primary camera. Select a camera entity and click \"Set as Primary\".";
        const ImVec2 textSize = ImGui::CalcTextSize(message);
        ImGui::SetCursorScreenPos(
            ImVec2(imagePos.x + (imageSize.x - textSize.x) * 0.5f, imagePos.y + (imageSize.y - textSize.y) * 0.5f));
        ImGui::TextUnformatted(message);
    }

    // Overlay control: promote the selected camera to primary.
    Entity selected = scene.FindEntityByUUID(context.selection.Primary());
    if (selected && selected.HasComponent<CameraComponent>()) {
        const bool isPrimary = selected.GetComponent<CameraComponent>().primary;
        ImGui::SetCursorScreenPos(ImVec2(imagePos.x + 8.0f, imagePos.y + 8.0f));
        ImGui::BeginDisabled(isPrimary);
        if (ImGui::Button(isPrimary ? "Primary Camera" : "Set as Primary")) {
            if (SetPrimaryCamera(scene, selected.GetUUID())) {
                context.MarkDirty();
            }
        }
        ImGui::EndDisabled();
    }
}

} // namespace Hockey
