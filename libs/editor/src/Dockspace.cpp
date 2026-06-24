#include "Hockey/Editor/Dockspace.hpp"

#include <imgui.h>

#include <imgui_internal.h>

namespace Hockey {

void Dockspace::BeginHost() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                       ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGui::Begin("##EditorDockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);
    m_HostOpen = true;
}

void Dockspace::SubmitDockSpace() {
    m_DockspaceId = ImGui::GetID("HockeyEditorDockSpace");

    const bool noExistingLayout = ImGui::DockBuilderGetNode(m_DockspaceId) == nullptr;
    if (m_ResetRequested || (m_FirstFrame && noExistingLayout)) {
        BuildDefaultLayout();
        m_ResetRequested = false;
    }
    m_FirstFrame = false;

    ImGui::DockSpace(m_DockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
}

void Dockspace::EndHost() {
    if (m_HostOpen) {
        ImGui::End();
        m_HostOpen = false;
    }
}

void Dockspace::BuildDefaultLayout() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(m_DockspaceId);
    ImGui::DockBuilderAddNode(m_DockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(m_DockspaceId, viewport->WorkSize);

    // Center starts as the whole space; each split carves a side off it.
    ImGuiID center = m_DockspaceId;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);

    using namespace EditorPanelNames;
    ImGui::DockBuilderDockWindow(kHierarchy, left);
    ImGui::DockBuilderDockWindow(kInspector, right);
    ImGui::DockBuilderDockWindow(kSceneViewport, center);
    ImGui::DockBuilderDockWindow(kGameViewport, center);
    ImGui::DockBuilderDockWindow(kProject, bottom);
    ImGui::DockBuilderDockWindow(kConsole, bottom);
    ImGui::DockBuilderDockWindow(kStats, bottom);
    ImGui::DockBuilderDockWindow(kSceneValidation, bottom);
    ImGui::DockBuilderDockWindow(kGameplayTuning, bottom);

    ImGui::DockBuilderFinish(m_DockspaceId);
}

} // namespace Hockey
