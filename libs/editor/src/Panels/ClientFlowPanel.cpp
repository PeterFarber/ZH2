#include "Hockey/Editor/Panels/ClientFlowPanel.hpp"

#include <cstdio>

#include <imgui.h>

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/UI/ClientFlowSerializer.hpp"
#include "Hockey/UI/UISettings.hpp"

namespace Hockey {
namespace {

void CopyToBuffer(std::array<char, 256>& buffer, const std::string& value) {
    std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
}

std::filesystem::path DefaultFlowPath(EditorContext& context) {
    if (!context.clientFlowAssetPath.empty()) {
        return context.clientFlowAssetPath;
    }
    if (context.config != nullptr) {
        const UISettings settings = LoadUISettings(*context.config);
        return Paths::Resolve(settings.startFlow);
    }
    return Paths::DataFile("ui/flows/default.clientflow.yaml");
}

} // namespace

ClientFlowPanel::ClientFlowPanel() : Panel(EditorPanelNames::kClientFlow, /*openByDefault=*/false) {
    CopyFlowToBuffers();
}

void ClientFlowPanel::OnImGui(EditorContext& context) {
    if (BeginWindow()) {
        const std::filesystem::path desiredPath = DefaultFlowPath(context);
        if (m_Path.empty() || m_Path != desiredPath) {
            LoadFromPath(desiredPath);
        }

        ImGui::TextUnformatted("Client Flow");
        ImGui::TextDisabled("%s", m_Path.empty() ? "<not selected>" : m_Path.string().c_str());
        DrawPathField("Home Screen", m_Home);
        DrawPathField("Match HUD", m_Hud);
        DrawPathField("Offline Scene", m_OfflineScene);

        if (ImGui::Button("Save")) {
            CopyBuffersToFlow();
            if (const Status status = ClientFlowSerializer::Save(m_Flow, m_Path); !status) {
                m_Status = status.error;
            } else {
                m_Status = "Saved Client Flow";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            LoadFromPath(m_Path);
        }
        ImGui::SameLine();
        if (ImGui::Button("Set As Client Startup")) {
            if (context.config != nullptr) {
                context.config->SetString("ui.start_flow", m_Path.lexically_relative(Paths::Get().root).generic_string());
                if (const Status status = context.config->Save(context.configPath); !status) {
                    m_Status = status.error;
                } else {
                    m_Status = "Client startup flow updated";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Client")) {
            context.clientFlowAssetPath = m_Path;
            context.requestClientPreviewStart = true;
            context.requestedPanelFocus = EditorPanelNames::kClientPreview;
        }

        if (!m_Status.empty()) {
            ImGui::Separator();
            ImGui::TextUnformatted(m_Status.c_str());
        }
    }
    EndWindow();
}

void ClientFlowPanel::LoadFromPath(const std::filesystem::path& path) {
    m_Path = path;
    if (const Result<ClientFlow> loaded = ClientFlowSerializer::Load(path)) {
        m_Flow = loaded.value;
        m_Status.clear();
    } else {
        m_Flow = MakeDefaultClientFlow();
        m_Status = loaded.error;
    }
    CopyFlowToBuffers();
}

void ClientFlowPanel::CopyFlowToBuffers() {
    CopyToBuffer(m_Home, m_Flow.screens.home);
    CopyToBuffer(m_Hud, m_Flow.screens.matchHud);
    CopyToBuffer(m_OfflineScene, m_Flow.offline.defaultScene);
}

void ClientFlowPanel::CopyBuffersToFlow() {
    m_Flow.screens.home = m_Home.data();
    m_Flow.screens.matchHud = m_Hud.data();
    m_Flow.offline.defaultScene = m_OfflineScene.data();
}

void ClientFlowPanel::DrawPathField(const char* label, std::array<char, 256>& buffer) {
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(label, buffer.data(), buffer.size());
}

} // namespace Hockey
