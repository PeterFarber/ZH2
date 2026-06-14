#include "Hockey/Editor/Panels/GameViewportPanel.hpp"

#include <algorithm>
#include <cstdint>

#include <imgui.h>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
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

} // namespace

GameViewportPanel::GameViewportPanel() : Panel(EditorPanelNames::kGameViewport) {}

void GameViewportPanel::OnImGui(EditorContext& context) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = BeginWindow();
    ImGui::PopStyleVar();
    if (visible) {
        DrawViewport(context);
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

void GameViewportPanel::DrawViewport(EditorContext& context) {
    Renderer* renderer = context.renderer;
    ImGuiRendererBridge* bridge = context.imguiBridge;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const auto width = static_cast<std::uint32_t>(std::max(1.0f, available.x));
    const auto height = static_cast<std::uint32_t>(std::max(1.0f, available.y));

    if (renderer == nullptr || bridge == nullptr || context.activeScene == nullptr || width < 8 || height < 8) {
        ImGui::TextUnformatted("Game viewport unavailable.");
        return;
    }

    Scene& scene = *context.activeScene;
    const ImVec2 imagePos = ImGui::GetCursorScreenPos();
    const ImVec2 imageSize(static_cast<float>(width), static_cast<float>(height));
    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    CameraRenderData camera;
    const bool hasCamera = FindActiveCamera(scene, aspect, camera);

    if (hasCamera) {
        EnsureTarget(context, width, height);
        // The game preview is a clean camera view: drop any editor debug lines
        // (grid/selection) submitted by the scene viewport so they don't leak in.
        renderer->Debug().Clear();
        renderer->RenderSceneToTarget(scene, camera, m_Target);

        const std::uint64_t textureId = m_Target.IsValid() ? bridge->ViewportTextureId(m_Target) : 0;
        if (textureId != 0) {
            ImGui::Image(static_cast<ImTextureID>(textureId), imageSize);
        } else {
            ImGui::Dummy(imageSize);
        }
    } else {
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
