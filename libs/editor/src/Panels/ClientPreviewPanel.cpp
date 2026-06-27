#include "Hockey/Editor/Panels/ClientPreviewPanel.hpp"

#include <algorithm>

#include <imgui.h>

#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorClientPreview.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/ImGui/ImGuiRendererBridge.hpp"
#include "Hockey/Editor/ViewportFrame.hpp"
#include "Hockey/Renderer/Renderer.hpp"
#include "Hockey/Renderer/Texture.hpp"

namespace Hockey {

ClientPreviewPanel::ClientPreviewPanel() : Panel(EditorPanelNames::kClientPreview, /*openByDefault=*/false) {}

void ClientPreviewPanel::OnImGui(EditorContext& context) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = BeginWindow();
    ImGui::PopStyleVar();
    if (!visible) {
        context.clientPreview.gameInputActive = false;
        EndWindow();
        return;
    }

    if (context.renderer == nullptr || context.imguiBridge == nullptr || context.clientPreviewHost == nullptr ||
        !context.clientPreviewHost->IsActive()) {
        ImGui::TextUnformatted("Client Preview is not running.");
        context.clientPreview.gameInputActive = false;
        EndWindow();
        return;
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 basePos = ImGui::GetCursorScreenPos();
    const EditorViewport::Frame frame = EditorViewport::ComputeFrame(basePos, available, context.renderer->GetSettings());
    if (frame.width < 8 || frame.height < 8) {
        ImGui::TextUnformatted("Client Preview unavailable.");
        context.clientPreview.gameInputActive = false;
        EndWindow();
        return;
    }

    EnsureTarget(context, frame.width, frame.height);
    context.clientPreview.viewportWidth = frame.width;
    context.clientPreview.viewportHeight = frame.height;
    context.clientPreviewHost->RenderToTarget(context, m_Target, frame.width, frame.height);

    const std::uint64_t textureId = m_Target.IsValid() ? context.imguiBridge->ViewportTextureId(m_Target) : 0;
    ImGui::SetCursorScreenPos(frame.imagePos);
    if (textureId != 0) {
        ImGui::Image(static_cast<ImTextureID>(textureId), frame.imageSize);
    } else {
        ImGui::Dummy(frame.imageSize);
    }

    const bool imageHovered = ImGui::IsItemHovered();
    context.clientPreview.gameInputActive = imageHovered && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (imageHovered) {
        const ImGuiIO& io = ImGui::GetIO();
        const int localX = static_cast<int>(std::clamp(io.MousePos.x - frame.imagePos.x, 0.0f, frame.imageSize.x));
        const int localY = static_cast<int>(std::clamp(io.MousePos.y - frame.imagePos.y, 0.0f, frame.imageSize.y));
        context.clientPreviewHost->HandlePointerInput(localX,
                                                      localY,
                                                      ImGui::IsMouseClicked(ImGuiMouseButton_Left),
                                                      ImGui::IsMouseReleased(ImGuiMouseButton_Left),
                                                      io.MouseWheel);
    }

    EndWindow();
}

void ClientPreviewPanel::EnsureTarget(EditorContext& context, std::uint32_t width, std::uint32_t height) {
    if (context.renderer == nullptr) {
        return;
    }
    if (!m_Target.IsValid()) {
        RenderTargetDesc desc;
        desc.width = width;
        desc.height = height;
        desc.debugName = "ClientPreview";
        m_Target = context.renderer->CreateRenderTarget(desc);
        m_Width = width;
        m_Height = height;
        return;
    }
    if (m_Width != width || m_Height != height) {
        context.renderer->ResizeRenderTarget(m_Target, width, height);
        if (context.imguiBridge != nullptr) {
            context.imguiBridge->InvalidateViewportTextures();
        }
        m_Width = width;
        m_Height = height;
    }
}

} // namespace Hockey
