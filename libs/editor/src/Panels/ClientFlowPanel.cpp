#include "Hockey/Editor/Panels/ClientFlowPanel.hpp"

#include <cstdio>
#include <vector>

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

bool HasParentTraversal(const std::filesystem::path& path) {
    for (const auto& part : path) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

bool IsProjectRelativeDataPath(const std::string& value) {
    const std::filesystem::path path(value);
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory() ||
        HasParentTraversal(path)) {
        return false;
    }
    const std::filesystem::path normalized = path.lexically_normal();
    auto it = normalized.begin();
    return it != normalized.end() && *it == "data";
}

bool FileExistsUnderProjectRoot(const std::string& value) {
    if (!IsProjectRelativeDataPath(value)) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_regular_file(Paths::Get().root / std::filesystem::path(value).lexically_normal(), ec);
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
        DrawPathField("Loading Screen", m_Loading);
        DrawPathField("Lobby Screen", m_Lobby);
        DrawPathField("Team Select", m_TeamSelect);
        DrawPathField("Match HUD", m_Hud);
        DrawPathField("Pause Menu", m_PauseMenu);
        DrawPathField("Settings Screen", m_Settings);
        DrawPathField("Scoreboard", m_Scoreboard);
        DrawPathField("End Match", m_EndMatch);
        ImGui::SeparatorText("Menu Background Scenes");
        DrawPathField("Home Background", m_HomeBackground);
        DrawPathField("Loading Background", m_LoadingBackground);
        DrawPathField("Lobby Background", m_LobbyBackground);
        DrawPathField("Team Select Background", m_TeamSelectBackground);
        DrawPathField("Pause Menu Background", m_PauseMenuBackground);
        DrawPathField("Settings Background", m_SettingsBackground);
        DrawPathField("Scoreboard Background", m_ScoreboardBackground);
        DrawPathField("End Match Background", m_EndMatchBackground);
        ImGui::SeparatorText("Offline Preview");
        DrawPathField("Offline Scene", m_OfflineScene);
        ImGui::Checkbox("Use Current Editor Scene", &m_UseCurrentEditorSceneWhenPreviewing);

        if (ImGui::Button("Save")) {
            CopyBuffersToFlow();
            if (!ValidateFlowForAuthoring()) {
                // m_Status is set by validation.
            } else if (const Status status = ClientFlowSerializer::Save(m_Flow, m_Path); !status) {
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
            CopyBuffersToFlow();
            if (ValidateFlowForAuthoring()) {
                context.clientFlowAssetPath = m_Path;
                context.requestClientPreviewStart = true;
                context.requestedPanelFocus = EditorPanelNames::kClientPreview;
            }
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
    CopyToBuffer(m_Loading, m_Flow.screens.loading);
    CopyToBuffer(m_Lobby, m_Flow.screens.lobby);
    CopyToBuffer(m_TeamSelect, m_Flow.screens.teamSelect);
    CopyToBuffer(m_Hud, m_Flow.screens.matchHud);
    CopyToBuffer(m_PauseMenu, m_Flow.screens.pauseMenu);
    CopyToBuffer(m_Settings, m_Flow.screens.settings);
    CopyToBuffer(m_Scoreboard, m_Flow.screens.scoreboard);
    CopyToBuffer(m_EndMatch, m_Flow.screens.endMatch);
    CopyToBuffer(m_HomeBackground, m_Flow.backgrounds.home);
    CopyToBuffer(m_LoadingBackground, m_Flow.backgrounds.loading);
    CopyToBuffer(m_LobbyBackground, m_Flow.backgrounds.lobby);
    CopyToBuffer(m_TeamSelectBackground, m_Flow.backgrounds.teamSelect);
    CopyToBuffer(m_PauseMenuBackground, m_Flow.backgrounds.pauseMenu);
    CopyToBuffer(m_SettingsBackground, m_Flow.backgrounds.settings);
    CopyToBuffer(m_ScoreboardBackground, m_Flow.backgrounds.scoreboard);
    CopyToBuffer(m_EndMatchBackground, m_Flow.backgrounds.endMatch);
    CopyToBuffer(m_OfflineScene, m_Flow.offline.defaultScene);
    m_UseCurrentEditorSceneWhenPreviewing = m_Flow.offline.useCurrentEditorSceneWhenPreviewing;
}

void ClientFlowPanel::CopyBuffersToFlow() {
    m_Flow.screens.home = m_Home.data();
    m_Flow.screens.loading = m_Loading.data();
    m_Flow.screens.lobby = m_Lobby.data();
    m_Flow.screens.teamSelect = m_TeamSelect.data();
    m_Flow.screens.matchHud = m_Hud.data();
    m_Flow.screens.pauseMenu = m_PauseMenu.data();
    m_Flow.screens.settings = m_Settings.data();
    m_Flow.screens.scoreboard = m_Scoreboard.data();
    m_Flow.screens.endMatch = m_EndMatch.data();
    m_Flow.backgrounds.home = m_HomeBackground.data();
    m_Flow.backgrounds.loading = m_LoadingBackground.data();
    m_Flow.backgrounds.lobby = m_LobbyBackground.data();
    m_Flow.backgrounds.teamSelect = m_TeamSelectBackground.data();
    m_Flow.backgrounds.pauseMenu = m_PauseMenuBackground.data();
    m_Flow.backgrounds.settings = m_SettingsBackground.data();
    m_Flow.backgrounds.scoreboard = m_ScoreboardBackground.data();
    m_Flow.backgrounds.endMatch = m_EndMatchBackground.data();
    m_Flow.offline.defaultScene = m_OfflineScene.data();
    m_Flow.offline.useCurrentEditorSceneWhenPreviewing = m_UseCurrentEditorSceneWhenPreviewing;
}

void ClientFlowPanel::DrawPathField(const char* label, std::array<char, 256>& buffer) {
    constexpr float kMinimumInputWidth = 320.0f;
    const float labelWidth = ImGui::CalcTextSize("Team Select Background").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushID(label);
    if (availableWidth >= labelWidth + kMinimumInputWidth) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(labelWidth);
    } else {
        ImGui::TextUnformatted(label);
    }
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##path", buffer.data(), buffer.size());
    ImGui::PopID();
}

bool ClientFlowPanel::ValidateFlowForAuthoring() {
    if (const std::vector<std::string> errors = ValidateClientFlow(m_Flow); !errors.empty()) {
        m_Status = errors.front();
        return false;
    }

    const std::pair<const char*, std::string> requiredFiles[] = {
        {"Home Screen", m_Flow.screens.home},
        {"Loading Screen", m_Flow.screens.loading},
        {"Lobby Screen", m_Flow.screens.lobby},
        {"Team Select", m_Flow.screens.teamSelect},
        {"Match HUD", m_Flow.screens.matchHud},
        {"Pause Menu", m_Flow.screens.pauseMenu},
        {"Settings Screen", m_Flow.screens.settings},
        {"Scoreboard", m_Flow.screens.scoreboard},
        {"End Match", m_Flow.screens.endMatch},
        {"Offline Scene", m_Flow.offline.defaultScene},
    };
    for (const auto& [label, path] : requiredFiles) {
        if (!FileExistsUnderProjectRoot(path)) {
            m_Status = std::string("Missing file: ") + label + " -> " + path;
            return false;
        }
    }

    const std::pair<const char*, std::string> optionalFiles[] = {
        {"Home Background", m_Flow.backgrounds.home},
        {"Loading Background", m_Flow.backgrounds.loading},
        {"Lobby Background", m_Flow.backgrounds.lobby},
        {"Team Select Background", m_Flow.backgrounds.teamSelect},
        {"Pause Menu Background", m_Flow.backgrounds.pauseMenu},
        {"Settings Background", m_Flow.backgrounds.settings},
        {"Scoreboard Background", m_Flow.backgrounds.scoreboard},
        {"End Match Background", m_Flow.backgrounds.endMatch},
    };
    for (const auto& [label, path] : optionalFiles) {
        if (!path.empty() && !FileExistsUnderProjectRoot(path)) {
            m_Status = std::string("Missing file: ") + label + " -> " + path;
            return false;
        }
    }

    m_Status.clear();
    return true;
}

} // namespace Hockey
