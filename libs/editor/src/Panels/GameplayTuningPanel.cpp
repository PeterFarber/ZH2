#include "Hockey/Editor/Panels/GameplayTuningPanel.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

#include <imgui.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"

namespace Hockey {
namespace {

bool DrawFloatValue(const char* label,
                    float& value,
                    float speed,
                    float minValue,
                    float maxValue,
                    const char* format = "%.3f") {
    const bool changed = ImGui::DragFloat(label, &value, speed, minValue, maxValue, format);
    if (changed) {
        value = std::clamp(value, minValue, maxValue);
    }
    return changed;
}

bool DrawUIntValue(const char* label, std::uint32_t& value, int minValue, int maxValue) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    const bool changed = ImGui::DragInt(label, &temp, 1.0f, minValue, maxValue);
    if (changed) {
        value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
    }
    return changed;
}

bool DrawVec3Value(const char* label, glm::vec3& value, float speed, float minValue, float maxValue) {
    float temp[3] = {value.x, value.y, value.z};
    const bool changed = ImGui::DragFloat3(label, temp, speed, minValue, maxValue, "%.3f");
    if (changed) {
        value.x = std::clamp(temp[0], minValue, maxValue);
        value.y = std::clamp(temp[1], minValue, maxValue);
        value.z = std::clamp(temp[2], minValue, maxValue);
    }
    return changed;
}

bool DrawPrefabPathValue(const char* label, std::filesystem::path& path) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", path.generic_string().c_str());

    bool changed = false;
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        path = buffer;
        changed = true;
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kPrefabDragDropType)) {
            path = std::filesystem::path(static_cast<const char*>(payload->Data));
            changed = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    const std::string clearId = std::string("Clear##") + label;
    if (ImGui::SmallButton(clearId.c_str())) {
        path.clear();
        changed = true;
    }

    return changed;
}

void LoadConfigOrDefault(const std::filesystem::path& path, Config& config, std::string& status) {
    config = Config{};
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        status = "Using defaults; missing " + path.filename().string();
        return;
    }
    if (const Status loaded = config.Load(path); !loaded) {
        status = loaded.error;
    }
}

} // namespace

GameplayTuningPanel::GameplayTuningPanel()
    : Panel(EditorPanelNames::kGameplayTuning, /*openByDefault=*/false) {}

void GameplayTuningPanel::EnsureLoaded(EditorContext& context) {
    if (!m_Loaded) {
        LoadAll(context);
    }
}

void GameplayTuningPanel::LoadAll(EditorContext& context) {
    m_TuningPath = Paths::DataFile("gameplay/tuning.default.yaml");
    m_EditorPath = context.configPath.empty() ? Paths::ConfigFile("editor.toml") : context.configPath;
    m_ClientPath = Paths::ConfigFile("client.toml");
    m_ServerPath = Paths::ConfigFile("server.toml");

    m_Status.clear();
    LoadConfigOrDefault(m_EditorPath, m_EditorConfig, m_Status);
    LoadConfigOrDefault(m_ClientPath, m_ClientConfig, m_Status);
    LoadConfigOrDefault(m_ServerPath, m_ServerConfig, m_Status);

    m_EditorSettings = LoadGameplaySettings(m_EditorConfig);
    m_ClientSettings = LoadGameplaySettings(m_ClientConfig);
    m_ServerSettings = LoadGameplaySettings(m_ServerConfig);

    if (const Result<GameplayTuning> loaded = TuningSerializer::Load(m_TuningPath)) {
        m_Tuning = loaded.value;
    } else {
        m_Tuning = GameplayTuning{};
        m_Status = "Gameplay tuning load failed: " + loaded.error;
    }

    m_TuningDirty = false;
    m_EditorDirty = false;
    m_ClientDirty = false;
    m_ServerDirty = false;
    m_Loaded = true;
}

void GameplayTuningPanel::DrawNavigation() {
    auto item = [this](const char* label, Section section) {
        if (ImGui::Selectable(label, m_Section == section)) {
            m_Section = section;
        }
    };
    item("Tuning YAML", Section::Tuning);
    item("Editor Preview", Section::EditorSettings);
    item("Client Build Defaults", Section::ClientSettings);
    item("Server Build Defaults", Section::ServerSettings);
}

void GameplayTuningPanel::DrawTuning(EditorContext& context) {
    bool changed = false;
    if (ImGui::CollapsingHeader("Skater", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Skater");
        changed |= DrawFloatValue("Max speed", m_Tuning.skater.maxSpeed, 0.05f, 0.0f, 80.0f);
        changed |= DrawFloatValue("Acceleration", m_Tuning.skater.acceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Deceleration", m_Tuning.skater.deceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Turn speed degrees", m_Tuning.skater.turnSpeedDegrees, 1.0f, 0.0f, 2160.0f);
        changed |= DrawFloatValue("Boost impulse", m_Tuning.skater.boostImpulse, 0.05f, 0.0f, 100.0f);
        changed |= DrawFloatValue("Boost cooldown seconds", m_Tuning.skater.boostCooldownSeconds, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Slide stop damping", m_Tuning.skater.slideStopDamping, 0.01f, 0.0f, 1.0f);
        changed |= DrawFloatValue("Double stop window seconds", m_Tuning.skater.doubleStopWindowSeconds, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Puck control radius", m_Tuning.skater.puckControlRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Steal radius", m_Tuning.skater.stealRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Steal cooldown seconds", m_Tuning.skater.stealCooldownSeconds, 0.01f, 0.0f, 30.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Goalie", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Goalie");
        changed |= DrawFloatValue("Max speed", m_Tuning.goalie.maxSpeed, 0.05f, 0.0f, 80.0f);
        changed |= DrawFloatValue("Acceleration", m_Tuning.goalie.acceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Crease move multiplier", m_Tuning.goalie.creaseMoveMultiplier, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Save reach radius", m_Tuning.goalie.saveReachRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawUIntValue("Boost charges", m_Tuning.goalie.boostCharges, 0, 10);
        changed |= DrawFloatValue("Boost recharge seconds", m_Tuning.goalie.boostRechargeSeconds, 0.01f, 0.0f, 60.0f);
        changed |= DrawFloatValue("Boost impulse", m_Tuning.goalie.boostImpulse, 0.05f, 0.0f, 100.0f);
        changed |= DrawFloatValue("Shield radius", m_Tuning.goalie.shieldRadius, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Shield duration seconds", m_Tuning.goalie.shieldDurationSeconds, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Shield cooldown seconds", m_Tuning.goalie.shieldCooldownSeconds, 0.01f, 0.0f, 60.0f);
        changed |= DrawFloatValue("Shield reflect impulse", m_Tuning.goalie.shieldReflectImpulse, 0.05f, 0.0f, 200.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Puck", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Puck");
        changed |= DrawFloatValue("Max speed", m_Tuning.puck.maxSpeed, 0.05f, 0.0f, 200.0f);
        changed |= DrawVec3Value("Possession offset", m_Tuning.puck.possessionOffset, 0.01f, -10.0f, 10.0f);
        changed |= DrawFloatValue("Loose puck drag", m_Tuning.puck.loosePuckDrag, 0.01f, 0.0f, 10.0f);
        changed |= DrawFloatValue("Floor Y", m_Tuning.puck.floorY, 0.01f, -10.0f, 10.0f);
        changed |= DrawFloatValue("Out of play Y", m_Tuning.puck.outOfPlayY, 0.05f, -200.0f, 50.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Stick", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Stick");
        changed |= DrawFloatValue("Reach", m_Tuning.stick.reach, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Width", m_Tuning.stick.width, 0.01f, 0.0f, 10.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Shot", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Shot");
        changed |= DrawFloatValue("Min power", m_Tuning.shot.minPower, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Max power", m_Tuning.shot.maxPower, 0.05f, 0.0f, 300.0f);
        changed |= DrawFloatValue("Charge seconds", m_Tuning.shot.chargeSeconds, 0.01f, 0.0f, 10.0f);
        changed |= DrawFloatValue("Self collision grace seconds", m_Tuning.shot.selfCollisionGraceSeconds, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Accuracy degrees", m_Tuning.shot.accuracyDegrees, 0.1f, 0.0f, 90.0f);
        ImGui::PopID();
    }
    if (changed) {
        m_TuningDirty = true;
    }
    if (ImGui::Button(m_TuningDirty ? "Save Tuning *" : "Save Tuning")) {
        SaveTuning(context);
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply To Preview")) {
        ApplyToPreview(context);
    }
}

void GameplayTuningPanel::DrawSettings(EditorContext& context, GameplaySettings& settings, const char* scope) {
    ImGui::PushID(scope);
    bool changed = false;
    changed |= ImGui::Checkbox("Enabled", &settings.enabled);
    changed |= DrawUIntValue("Target player count", settings.targetPlayerCount, 1, 16);
    changed |= DrawUIntValue("Skaters per team", settings.skatersPerTeam, 0, 8);
    changed |= DrawUIntValue("Goalies per team", settings.goaliesPerTeam, 0, 4);
    changed |= DrawFloatValue("Fixed delta seconds", settings.fixedDeltaSeconds, 0.0001f, 0.001f, 0.1f, "%.5f");
    changed |= DrawFloatValue("Period length seconds", settings.periodLengthSeconds, 1.0f, 1.0f, 7200.0f, "%.1f");
    changed |= DrawUIntValue("Period count", settings.periodCount, 1, 20);
    changed |= DrawFloatValue("Pregame countdown seconds", settings.pregameCountdownSeconds, 0.1f, 0.0f, 120.0f);
    changed |= DrawFloatValue("Countdown beep start seconds", settings.countdownBeepStartSeconds, 0.1f, 0.0f, 60.0f);
    changed |= ImGui::Checkbox("Stop clock after goal", &settings.stopClockAfterGoal);
    changed |= ImGui::Checkbox("Auto faceoff after goal", &settings.autoFaceoffAfterGoal);
    changed |= DrawFloatValue("Post goal delay seconds", settings.postGoalDelaySeconds, 0.1f, 0.0f, 120.0f);
    changed |= DrawFloatValue("Faceoff delay seconds", settings.faceoffDelaySeconds, 0.1f, 0.0f, 60.0f);
    changed |= DrawFloatValue("Goal detection radius", settings.goalDetectionRadius, 0.01f, 0.0f, 20.0f);
    changed |= ImGui::Checkbox("Require puck for goal", &settings.requirePuckForGoal);
    changed |= ImGui::Checkbox("Allow manual goalie", &settings.allowManualGoalie);
    changed |= ImGui::Checkbox("Allow out of play", &settings.allowOutOfPlay);
    changed |= DrawPrefabPathValue("Waypoint prefab", settings.waypointPrefabPath);
    changed |= ImGui::Checkbox("Debug draw gameplay", &settings.debugDrawGameplay);
    changed |= ImGui::Checkbox("Log gameplay events", &settings.logGameplayEvents);

    if (changed) {
        if (std::string(scope) == "Editor") {
            m_EditorDirty = true;
        } else if (std::string(scope) == "Client") {
            m_ClientDirty = true;
        } else {
            m_ServerDirty = true;
        }
    }

    const bool editorScope = std::string(scope) == "Editor";
    const bool clientScope = std::string(scope) == "Client";
    const bool dirty = editorScope ? m_EditorDirty : (clientScope ? m_ClientDirty : m_ServerDirty);
    std::string saveLabel;
    if (editorScope) {
        saveLabel = "Save Editor Preview";
    } else if (clientScope) {
        saveLabel = "Save Client Build Defaults";
    } else {
        saveLabel = "Save Server Build Defaults";
    }
    if (dirty) {
        saveLabel += " *";
    }
    if (ImGui::Button(saveLabel.c_str())) {
        if (editorScope) {
            SaveEditorConfig(context);
        } else if (clientScope) {
            SaveClientConfig();
        } else {
            SaveServerConfig();
        }
    }
    ImGui::PopID();
}

void GameplayTuningPanel::SaveTuning(EditorContext& context) {
    if (const Status status = TuningSerializer::Save(m_TuningPath, m_Tuning); !status) {
        m_Status = status.error;
        return;
    }
    m_TuningDirty = false;
    m_Status = "Saved " + m_TuningPath.filename().string();
    ApplyToPreview(context);
}

void GameplayTuningPanel::SaveEditorConfig(EditorContext& context) {
    SaveGameplaySettings(m_EditorConfig, m_EditorSettings);
    if (const Status status = m_EditorConfig.Save(m_EditorPath); !status) {
        m_Status = status.error;
        return;
    }
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    m_EditorDirty = false;
    m_Status = "Saved " + m_EditorPath.filename().string();
    ApplyToPreview(context);
}

void GameplayTuningPanel::SaveClientConfig() {
    SaveGameplaySettings(m_ClientConfig, m_ClientSettings);
    if (const Status status = m_ClientConfig.Save(m_ClientPath); !status) {
        m_Status = status.error;
        return;
    }
    m_ClientDirty = false;
    m_Status = "Saved " + m_ClientPath.filename().string();
}

void GameplayTuningPanel::SaveServerConfig() {
    SaveGameplaySettings(m_ServerConfig, m_ServerSettings);
    if (const Status status = m_ServerConfig.Save(m_ServerPath); !status) {
        m_Status = status.error;
        return;
    }
    m_ServerDirty = false;
    m_Status = "Saved " + m_ServerPath.filename().string();
}

void GameplayTuningPanel::ApplyToPreview(EditorContext& context) {
    if (context.gameplayPreview == nullptr) {
        m_Status = "Gameplay preview is not available";
        return;
    }
    context.gameplayPreview->Configure(m_EditorSettings, m_Tuning);
    context.gameplayPreview->SetDebugDrawEnabled(m_EditorSettings.debugDrawGameplay);
    if (context.gameplayPreview->IsActive() && context.activeScene != nullptr && context.physicsPreview != nullptr) {
        context.gameplayPreview->Reset(*context.activeScene, *context.physicsPreview);
    }
    m_Status = "Applied gameplay tuning to editor preview";
}

void GameplayTuningPanel::OnImGui(EditorContext& context) {
    EnsureLoaded(context);
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    if (!m_Status.empty()) {
        ImGui::TextWrapped("%s", m_Status.c_str());
        ImGui::Separator();
    }

    if (ImGui::Button("Reload From Disk")) {
        LoadAll(context);
        m_Status = "Reloaded gameplay tuning files";
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply To Preview")) {
        ApplyToPreview(context);
    }
    ImGui::Separator();

    ImGui::BeginChild("##gameplayTuningNav", ImVec2(180.0f, 0.0f), true);
    DrawNavigation();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##gameplayTuningBody", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    switch (m_Section) {
    case Section::Tuning:
        DrawTuning(context);
        break;
    case Section::EditorSettings:
        DrawSettings(context, m_EditorSettings, "Editor");
        break;
    case Section::ClientSettings:
        DrawSettings(context, m_ClientSettings, "Client");
        break;
    case Section::ServerSettings:
        DrawSettings(context, m_ServerSettings, "Server");
        break;
    }
    ImGui::EndChild();
    EndWindow();
}

} // namespace Hockey
