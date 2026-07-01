#include "Hockey/Editor/Panels/GameplayTuningPanel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>

#include <imgui.h>

#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/RuntimeConfigDefaults.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"

namespace Hockey {
namespace {

enum class SettingValueKind {
    Boolean,
    Integer,
    Float,
    Vector3,
    Path,
};

std::string FormatBool(bool value) {
    return value ? "On" : "Off";
}

std::string FormatFloat(float value, const char* format) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), format, value);
    return buffer;
}

std::string FormatVec3(const glm::vec3& value) {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", value.x, value.y, value.z);
    return buffer;
}

std::string FormatStringDefault(const std::string& value) {
    return value.empty() ? "empty" : value;
}

std::string BuildRangeText(int minValue, int maxValue) {
    return std::to_string(minValue) + " to " + std::to_string(maxValue);
}

std::string BuildRangeText(float minValue, float maxValue, const char* format) {
    return FormatFloat(minValue, format) + " to " + FormatFloat(maxValue, format);
}

std::string LabelText(const char* label) {
    return (label != nullptr && label[0] != '\0') ? label : "This gameplay setting";
}

std::string ToLowerText(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool ContainsAny(const std::string& text, std::initializer_list<const char*> needles) {
    for (const char* needle : needles) {
        if (needle != nullptr && text.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool HasTerminalPunctuation(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    const char last = text.back();
    return last == '.' || last == '!' || last == '?';
}

void AppendLabeledSentence(std::string& text, const char* label, const std::string& value) {
    text += "\n";
    text += label;
    text += value;
    if (!HasTerminalPunctuation(value)) {
        text += ".";
    }
}

std::string BuildGeneratedExamplesText(const char* label,
                                       SettingValueKind kind,
                                       const std::string& recommendedValue,
                                       const std::string& range) {
    const std::string name = LabelText(label);
    switch (kind) {
    case SettingValueKind::Boolean:
        return "Turn " + name + " on to enable that gameplay behavior in this scope; turn it off to disable it. The recommended reset state is " + recommendedValue;
    case SettingValueKind::Integer:
    case SettingValueKind::Float:
        if (!range.empty()) {
            return name + " at " + recommendedValue + " is the reset baseline. Moving toward the lower end of " + range + " reduces " + name + "'s reach, count, timing, or intensity; moving toward the higher end increases it";
        }
        return name + " at " + recommendedValue + " is the reset baseline. Smaller values reduce " + name + "'s effect; larger values increase it";
    case SettingValueKind::Vector3:
        return name + " at " + recommendedValue + " is the reset baseline. Adjust X/Y/Z independently to move the gameplay attachment or possession point in local space";
    case SettingValueKind::Path:
        return "Leave " + name + " at " + recommendedValue + " to use the recommended authored prefab. Override it with a project-relative prefab path when testing a different stick, waypoint, or helper asset";
    }
    return name + " uses " + recommendedValue + " as the reset baseline";
}

std::string BuildGeneratedPerformanceImpactText(const char* label,
                                                SettingValueKind kind,
                                                const char* description) {
    const std::string name = LabelText(label);
    const std::string search = ToLowerText(name + " " + ((description != nullptr) ? description : ""));
    if (ContainsAny(search, {"fixed delta", "tick", "period", "countdown", "delay", "cooldown", "charge",
                             "recharge", "duration", "seconds"})) {
        return name + " affects gameplay timing. Shorter intervals or higher tick rates can run logic more often and cost more CPU; longer timings usually reduce frequency but change match pacing";
    }
    if (ContainsAny(search, {"speed", "acceleration", "deceleration", "turn", "boost", "impulse", "power",
                             "damping", "drag"})) {
        return name + " changes movement or shot energy. The value is cheap to read, but extreme values can make physics contacts, possession, and gameplay prediction harder to tune";
    }
    if (ContainsAny(search, {"radius", "reach", "width", "offset", "accuracy", "goal detection", "control",
                             "steal", "shield", "collision"})) {
        return name + " changes spatial gameplay checks. Wider radii or larger reach values can increase overlap/query work and make interactions more permissive; tighter values are cheaper and stricter";
    }
    if (ContainsAny(search, {"player", "skater", "goalie", "charges", "team"})) {
        return name + " changes actor capacity or role behavior. Higher counts can increase simulation and setup work; lower counts are cheaper but may not match the 4v4 design";
    }
    if (ContainsAny(search, {"prefab", "waypoint", "path"})) {
        return name + " can affect prefab instantiation and asset loading. Valid recommended paths keep preview setup predictable; missing or heavy prefabs can add load/debug time";
    }
    if (ContainsAny(search, {"debug", "draw", "log"})) {
        return name + " can add editor rendering, logging, or diagnostic work. Enable it while diagnosing gameplay and turn it off for clean performance checks";
    }
    if (kind == SettingValueKind::Boolean) {
        return name + " costs only when the enabled gameplay behavior performs work during preview or server ticks";
    }
    return name + " is read by gameplay tuning or settings code; conservative values keep simulation cost predictable while extreme values can stress movement, queries, logging, or setup";
}

std::string BuildSettingTooltip(const char* label, SettingValueKind kind, const char* description, const std::string& recommendedValue,
                                const std::string& range = {}, const char* examples = nullptr,
                                const char* performanceImpact = nullptr) {
    std::string text = (description != nullptr && description[0] != '\0') ? description
                                                                           : "Adjusts this gameplay setting.";
    text += "\n";
    AppendLabeledSentence(text, "Recommended value: ", recommendedValue);
    if (!range.empty()) {
        AppendLabeledSentence(text, "Allowed range: ", range);
    }
    AppendLabeledSentence(text, "Examples: ",
                          (examples != nullptr && examples[0] != '\0')
                              ? examples
                              : BuildGeneratedExamplesText(label, kind, recommendedValue, range));
    AppendLabeledSentence(text, "Performance impact: ",
                          (performanceImpact != nullptr && performanceImpact[0] != '\0')
                              ? performanceImpact
                              : BuildGeneratedPerformanceImpactText(label, kind, description));
    return text;
}

void ShowSettingTooltip(const char* label, SettingValueKind kind, const char* description, const std::string& recommendedValue,
                        const std::string& range = {}, const char* examples = nullptr,
                        const char* performanceImpact = nullptr) {
    const std::string tooltip =
        BuildSettingTooltip(label, kind, description, recommendedValue, range, examples, performanceImpact);
    EditorTooltip::ForLastItem(tooltip.c_str());
}

bool DrawResetSettingButton(const char* label) {
    ImGui::SameLine();
    const std::string resetId = std::string("Reset Setting##") + label;
    const bool reset = ImGui::SmallButton(resetId.c_str());
    EditorTooltip::ForLastItem("Reset this setting to the built-in default.");
    return reset;
}

bool DrawFloatValue(const char* label,
                    float& value,
                    float defaultValue,
                    float speed,
                    float minValue,
                    float maxValue,
                    const char* format = "%.3f",
                    const char* description = nullptr,
                    const char* examples = nullptr,
                    const char* performanceImpact = nullptr) {
    const bool changed = ImGui::DragFloat(label, &value, speed, minValue, maxValue, format);
    ShowSettingTooltip(label, SettingValueKind::Float, description, FormatFloat(defaultValue, format),
                       BuildRangeText(minValue, maxValue, format), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = std::clamp(defaultValue, minValue, maxValue);
        return true;
    }
    if (changed) {
        value = std::clamp(value, minValue, maxValue);
    }
    return changed;
}

bool DrawUIntValue(const char* label,
                   std::uint32_t& value,
                   std::uint32_t defaultValue,
                   int minValue,
                   int maxValue,
                   const char* description = nullptr,
                   const char* examples = nullptr,
                   const char* performanceImpact = nullptr) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    const bool changed = ImGui::DragInt(label, &temp, 1.0f, minValue, maxValue);
    ShowSettingTooltip(label, SettingValueKind::Integer, description, std::to_string(defaultValue),
                       BuildRangeText(minValue, maxValue), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = static_cast<std::uint32_t>(std::clamp(static_cast<int>(defaultValue), minValue, maxValue));
        return true;
    }
    if (changed) {
        value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
    }
    return changed;
}

bool DrawVec3Value(const char* label, glm::vec3& value, const glm::vec3& defaultValue, float speed, float minValue,
                   float maxValue, const char* description = nullptr, const char* examples = nullptr,
                   const char* performanceImpact = nullptr) {
    float temp[3] = {value.x, value.y, value.z};
    const bool changed = ImGui::DragFloat3(label, temp, speed, minValue, maxValue, "%.3f");
    ShowSettingTooltip(label, SettingValueKind::Vector3, description, FormatVec3(defaultValue),
                       "each component " + BuildRangeText(minValue, maxValue, "%.3f"), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value.x = std::clamp(defaultValue.x, minValue, maxValue);
        value.y = std::clamp(defaultValue.y, minValue, maxValue);
        value.z = std::clamp(defaultValue.z, minValue, maxValue);
        return true;
    }
    if (changed) {
        value.x = std::clamp(temp[0], minValue, maxValue);
        value.y = std::clamp(temp[1], minValue, maxValue);
        value.z = std::clamp(temp[2], minValue, maxValue);
    }
    return changed;
}

bool DrawBoolValue(const char* label,
                   bool& value,
                   bool defaultValue,
                   const char* description = nullptr,
                   const char* examples = nullptr,
                   const char* performanceImpact = nullptr) {
    const bool changed = ImGui::Checkbox(label, &value);
    ShowSettingTooltip(label, SettingValueKind::Boolean, description, FormatBool(defaultValue), {}, examples,
                       performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = defaultValue;
        return true;
    }
    return changed;
}

bool DrawPrefabPathValue(const char* label,
                         std::filesystem::path& path,
                         const std::filesystem::path& defaultPath,
                         const char* description = nullptr,
                         const char* examples = nullptr,
                         const char* performanceImpact = nullptr) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", path.generic_string().c_str());

    bool changed = false;
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        path = buffer;
        changed = true;
    }
    ShowSettingTooltip(label, SettingValueKind::Path, description, FormatStringDefault(defaultPath.generic_string()), {},
                       examples, performanceImpact);

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
    EditorTooltip::ForLastItem("Clears this prefab path so gameplay keeps the underlying behavior but stops spawning "
                               "the visible helper prefab. Use Reset to restore the recommended prefab.");
    if (DrawResetSettingButton(label)) {
        path = defaultPath;
        changed = true;
    }

    return changed;
}

void LoadConfigOverlay(const std::filesystem::path& path, Config& config, std::string& status) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        status = "Using defaults; missing " + path.filename().string();
        return;
    }
    Config overlay;
    if (const Status loaded = overlay.Load(path); !loaded) {
        status = loaded.error;
        return;
    }
    config.MergeFrom(overlay);
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

    m_Status.clear();
    m_DefaultConfig = MakeDefaultRuntimeConfig();
    m_EditorConfig = m_DefaultConfig;
    LoadConfigOverlay(m_EditorPath, m_EditorConfig, m_Status);
    m_ServerConfig = m_EditorConfig;

    m_DefaultSettings = LoadGameplaySettings(m_DefaultConfig);
    m_EditorSettings = LoadGameplaySettings(m_EditorConfig);
    m_ServerSettings = LoadGameplaySettings(m_ServerConfig);

    if (const Result<GameplayTuning> loaded = TuningSerializer::Load(m_TuningPath)) {
        m_Tuning = loaded.value;
    } else {
        m_Tuning = GameplayTuning{};
        m_Status = "Gameplay tuning load failed: " + loaded.error;
    }

    m_TuningDirty = false;
    m_EditorDirty = false;
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
    item("Server Build Defaults", Section::ServerSettings);
}

void GameplayTuningPanel::DrawTuning(EditorContext& context) {
    const GameplayTuning defaults;
    if (ImGui::Button("Reset Tuning To Defaults")) {
        m_Tuning = defaults;
        m_TuningDirty = true;
    }
    ImGui::Separator();

    bool changed = false;
    if (ImGui::CollapsingHeader("Skater", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Skater");
        changed |= DrawFloatValue("Max speed", m_Tuning.skater.maxSpeed, defaults.skater.maxSpeed, 0.05f, 0.0f, 80.0f,
                                  "%.3f", "Maximum skating speed for controlled skaters in gameplay tuning.",
                                  "Higher skater max speed makes rushes and breakaways faster; lower values make defense easier and reduce how quickly players cover the rink.",
                                  "Speed changes are cheap by themselves, but very high speeds can make collisions and puck possession harder to stabilize at the fixed timestep.");
        changed |= DrawFloatValue("Acceleration", m_Tuning.skater.acceleration, defaults.skater.acceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Deceleration", m_Tuning.skater.deceleration, defaults.skater.deceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Turn speed degrees", m_Tuning.skater.turnSpeedDegrees, defaults.skater.turnSpeedDegrees, 1.0f, 0.0f, 2160.0f);
        changed |= DrawFloatValue("Boost impulse", m_Tuning.skater.boostImpulse, defaults.skater.boostImpulse, 0.05f, 0.0f, 100.0f);
        changed |= DrawFloatValue("Boost cooldown seconds", m_Tuning.skater.boostCooldownSeconds, defaults.skater.boostCooldownSeconds, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Slide stop damping", m_Tuning.skater.slideStopDamping, defaults.skater.slideStopDamping, 0.01f, 0.0f, 1.0f);
        changed |= DrawFloatValue("Double stop window seconds", m_Tuning.skater.doubleStopWindowSeconds, defaults.skater.doubleStopWindowSeconds, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Puck control radius", m_Tuning.skater.puckControlRadius, defaults.skater.puckControlRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Steal radius", m_Tuning.skater.stealRadius, defaults.skater.stealRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Steal cooldown seconds", m_Tuning.skater.stealCooldownSeconds, defaults.skater.stealCooldownSeconds, 0.01f, 0.0f, 30.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Goalie", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Goalie");
        changed |= DrawFloatValue("Max speed", m_Tuning.goalie.maxSpeed, defaults.goalie.maxSpeed, 0.05f, 0.0f, 80.0f);
        changed |= DrawFloatValue("Acceleration", m_Tuning.goalie.acceleration, defaults.goalie.acceleration, 0.05f, 0.0f, 200.0f);
        changed |= DrawFloatValue("Crease move multiplier", m_Tuning.goalie.creaseMoveMultiplier, defaults.goalie.creaseMoveMultiplier, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Save reach radius", m_Tuning.goalie.saveReachRadius, defaults.goalie.saveReachRadius, 0.01f, 0.0f, 20.0f);
        changed |= DrawUIntValue("Boost charges", m_Tuning.goalie.boostCharges, defaults.goalie.boostCharges, 0, 10);
        changed |= DrawFloatValue("Boost recharge seconds", m_Tuning.goalie.boostRechargeSeconds, defaults.goalie.boostRechargeSeconds, 0.01f, 0.0f, 60.0f);
        changed |= DrawFloatValue("Boost impulse", m_Tuning.goalie.boostImpulse, defaults.goalie.boostImpulse, 0.05f, 0.0f, 100.0f);
        changed |= DrawFloatValue("Shield radius", m_Tuning.goalie.shieldRadius, defaults.goalie.shieldRadius, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Shield duration seconds", m_Tuning.goalie.shieldDurationSeconds, defaults.goalie.shieldDurationSeconds, 0.01f, 0.0f, 30.0f);
        changed |= DrawFloatValue("Shield cooldown seconds", m_Tuning.goalie.shieldCooldownSeconds, defaults.goalie.shieldCooldownSeconds, 0.01f, 0.0f, 60.0f);
        changed |= DrawFloatValue("Shield reflect impulse", m_Tuning.goalie.shieldReflectImpulse, defaults.goalie.shieldReflectImpulse, 0.05f, 0.0f, 200.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Puck", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Puck");
        changed |= DrawFloatValue("Max speed", m_Tuning.puck.maxSpeed, defaults.puck.maxSpeed, 0.05f, 0.0f,
                                  200.0f);
        changed |= DrawVec3Value("Possession offset", m_Tuning.puck.possessionOffset,
                                 defaults.puck.possessionOffset, 0.01f, -10.0f, 10.0f);
        changed |= DrawFloatValue("Loose puck drag", m_Tuning.puck.loosePuckDrag, defaults.puck.loosePuckDrag,
                                  0.01f, 0.0f, 10.0f);
        changed |= DrawFloatValue("Floor Y", m_Tuning.puck.floorY, defaults.puck.floorY, 0.01f, -10.0f, 10.0f);
        changed |= DrawFloatValue("Out of play Y", m_Tuning.puck.outOfPlayY, defaults.puck.outOfPlayY, 0.05f,
                                  -200.0f, 50.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Skater Stick", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("SkaterStick");
        changed |= DrawPrefabPathValue("Prefab", m_Tuning.skaterStick.prefabPath, defaults.skaterStick.prefabPath);
        changed |= DrawFloatValue("Reach", m_Tuning.skaterStick.reach, defaults.skaterStick.reach, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Width", m_Tuning.skaterStick.width, defaults.skaterStick.width, 0.01f, 0.0f, 10.0f);
        changed |= DrawVec3Value("Local offset", m_Tuning.skaterStick.localOffset, defaults.skaterStick.localOffset,
                                 0.01f, -10.0f, 10.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Goalie Stick", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("GoalieStick");
        changed |= DrawPrefabPathValue("Prefab", m_Tuning.goalieStick.prefabPath, defaults.goalieStick.prefabPath);
        changed |= DrawFloatValue("Reach", m_Tuning.goalieStick.reach, defaults.goalieStick.reach, 0.01f, 0.0f, 20.0f);
        changed |= DrawFloatValue("Width", m_Tuning.goalieStick.width, defaults.goalieStick.width, 0.01f, 0.0f, 10.0f);
        changed |= DrawVec3Value("Local offset", m_Tuning.goalieStick.localOffset, defaults.goalieStick.localOffset,
                                 0.01f, -10.0f, 10.0f);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Shot", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Shot");
        changed |= DrawFloatValue("Min power", m_Tuning.shot.minPower, defaults.shot.minPower, 0.05f, 0.0f,
                                  200.0f);
        changed |= DrawFloatValue("Max power", m_Tuning.shot.maxPower, defaults.shot.maxPower, 0.05f, 0.0f,
                                  300.0f);
        changed |= DrawFloatValue("Charge seconds", m_Tuning.shot.chargeSeconds, defaults.shot.chargeSeconds, 0.01f,
                                  0.0f, 10.0f, "%.3f",
                                  "Time required to reach full shot charge from the minimum shot power.",
                                  "Longer shot charge raises commitment and lowers shot spam; shorter values make quick snapshots easier; 0.0 makes shots immediate.",
                                  "Charge time has no direct rendering cost, but very short values can increase shot-event frequency during gameplay tests.");
        changed |= DrawFloatValue("Self collision grace seconds", m_Tuning.shot.selfCollisionGraceSeconds,
                                  defaults.shot.selfCollisionGraceSeconds, 0.01f, 0.0f, 5.0f);
        changed |= DrawFloatValue("Accuracy degrees", m_Tuning.shot.accuracyDegrees, defaults.shot.accuracyDegrees,
                                  0.1f, 0.0f, 90.0f);
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
    if (ImGui::Button("Reset Settings To Defaults")) {
        settings = m_DefaultSettings;
        if (std::string(scope) == "Editor") {
            m_EditorDirty = true;
        } else {
            m_ServerDirty = true;
        }
    }
    ImGui::Separator();

    bool changed = false;
    changed |= DrawBoolValue("Enabled", settings.enabled, m_DefaultSettings.enabled,
                             "Enables gameplay systems for this scope, including match flow, scoring, possession, and preview input.",
                             "On runs gameplay rules in this scope; Off leaves the scene available for layout or renderer-only checks.",
                             "Turning it off skips gameplay tick work; turning it on adds normal fixed-step gameplay simulation.");
    changed |= DrawUIntValue("Target player count", settings.targetPlayerCount, m_DefaultSettings.targetPlayerCount, 1,
                             16);
    changed |= DrawUIntValue("Skaters per team", settings.skatersPerTeam, m_DefaultSettings.skatersPerTeam, 0, 8);
    changed |= DrawUIntValue("Goalies per team", settings.goaliesPerTeam, m_DefaultSettings.goaliesPerTeam, 0, 4);
    changed |= DrawFloatValue("Fixed delta seconds", settings.fixedDeltaSeconds, m_DefaultSettings.fixedDeltaSeconds,
                              0.0001f, 0.001f, 0.1f, "%.5f",
                              "Gameplay fixed delta controls how often rules, possession, scoring, and preview input tick.",
                              "0.0083 runs gameplay at roughly 120 Hz; 0.0167 is roughly 60 Hz; 0.0333 is roughly 30 Hz and reduces CPU cost but makes fast puck and possession events less precise.",
                              "Smaller values run more gameplay ticks per second and increase CPU cost; larger values reduce tick count but lower simulation fidelity.");
    changed |= DrawFloatValue("Period length seconds", settings.periodLengthSeconds,
                              m_DefaultSettings.periodLengthSeconds, 1.0f, 1.0f, 7200.0f, "%.1f");
    changed |= DrawUIntValue("Period count", settings.periodCount, m_DefaultSettings.periodCount, 1, 20);
    changed |= DrawFloatValue("Pregame countdown seconds", settings.pregameCountdownSeconds,
                              m_DefaultSettings.pregameCountdownSeconds, 0.1f, 0.0f, 120.0f);
    changed |= DrawFloatValue("Countdown beep start seconds", settings.countdownBeepStartSeconds,
                              m_DefaultSettings.countdownBeepStartSeconds, 0.1f, 0.0f, 60.0f);
    changed |= DrawBoolValue("Stop clock after goal", settings.stopClockAfterGoal,
                             m_DefaultSettings.stopClockAfterGoal,
                             "Stops the game clock after goals so celebration and reset timing can play without draining period time.",
                             "On creates a pause after scoring; Off keeps the clock running for faster arcade pacing.",
                             "No meaningful performance cost; it only changes match-flow state transitions.");
    changed |= DrawBoolValue("Auto faceoff after goal", settings.autoFaceoffAfterGoal,
                             m_DefaultSettings.autoFaceoffAfterGoal,
                             "Automatically returns players and puck to faceoff positions after a goal.",
                             "On keeps tests and previews moving without manual reset; Off lets you inspect post-goal positions.",
                             "No meaningful performance cost beyond the reset work when a goal occurs.");
    changed |= DrawFloatValue("Post goal delay seconds", settings.postGoalDelaySeconds,
                              m_DefaultSettings.postGoalDelaySeconds, 0.1f, 0.0f, 120.0f);
    changed |= DrawFloatValue("Faceoff delay seconds", settings.faceoffDelaySeconds,
                              m_DefaultSettings.faceoffDelaySeconds, 0.1f, 0.0f, 60.0f);
    changed |= DrawFloatValue("Goal detection radius", settings.goalDetectionRadius,
                              m_DefaultSettings.goalDetectionRadius, 0.01f, 0.0f, 20.0f);
    changed |= DrawBoolValue("Require puck for goal", settings.requirePuckForGoal,
                             m_DefaultSettings.requirePuckForGoal,
                             "Requires scoring triggers to be caused by the puck instead of arbitrary entities.",
                             "On prevents skaters, props, or debug bodies from scoring; Off is useful for trigger debugging.",
                             "No meaningful performance cost; it changes goal validation logic.");
    changed |= DrawBoolValue("Allow manual goalie", settings.allowManualGoalie,
                             m_DefaultSettings.allowManualGoalie,
                             "Allows a player-controlled goalie role when the scene has goalie spawns.",
                             "On supports manual goalie testing; Off keeps goalie behavior controlled by default systems.",
                             "No meaningful performance cost; it changes role assignment rules.");
    changed |= DrawBoolValue("Allow out of play", settings.allowOutOfPlay, m_DefaultSettings.allowOutOfPlay,
                             "Allows puck and player state to enter out-of-play handling when leaving legal areas.",
                             "On exercises reset and boundary rules; Off keeps objects live for freeform preview movement.",
                             "No meaningful performance cost unless repeated out-of-play resets are happening.");
    changed |= DrawPrefabPathValue("Waypoint prefab", settings.waypointPrefabPath,
                                   m_DefaultSettings.waypointPrefabPath);
    changed |= DrawBoolValue("Debug draw gameplay", settings.debugDrawGameplay,
                             m_DefaultSettings.debugDrawGameplay,
                             "Draws gameplay markers, zones, and role debug information over the scene.",
                             "On helps author spawn pools, faceoff spots, and puck interactions; Off keeps art review clean.",
                             "Debug draw adds editor rendering work because zones, markers, and gameplay overlays are submitted on top of the scene.");
    changed |= DrawBoolValue("Log gameplay events", settings.logGameplayEvents,
                             m_DefaultSettings.logGameplayEvents,
                             "Logs gameplay events such as goals, faceoffs, possession changes, and rule transitions.",
                             "On gives detailed diagnostics while tuning; Off keeps the log quieter during normal editing.",
                             "Logging can add CPU and I/O work when many gameplay events fire in quick succession.");

    if (changed) {
        if (std::string(scope) == "Editor") {
            m_EditorDirty = true;
        } else {
            m_ServerDirty = true;
        }
    }

    const bool editorScope = std::string(scope) == "Editor";
    const bool dirty = editorScope ? m_EditorDirty : m_ServerDirty;
    std::string saveLabel = editorScope ? "Save Editor Preview" : "Save Server Build Defaults";
    if (dirty) {
        saveLabel += " *";
    }
    if (ImGui::Button(saveLabel.c_str())) {
        if (editorScope) {
            SaveEditorConfig(context);
        } else {
            SaveServerConfig(context);
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
    m_ServerConfig = m_EditorConfig;
    m_ServerSettings = m_EditorSettings;
    m_EditorDirty = false;
    m_Status = "Saved " + m_EditorPath.filename().string();
    ApplyToPreview(context);
}

void GameplayTuningPanel::SaveServerConfig(EditorContext& context) {
    SaveGameplaySettings(m_ServerConfig, m_ServerSettings);
    m_EditorConfig = m_ServerConfig;
    if (const Status status = m_EditorConfig.Save(m_EditorPath); !status) {
        m_Status = status.error;
        return;
    }
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    m_EditorSettings = m_ServerSettings;
    m_ServerDirty = false;
    m_Status = "Saved " + m_EditorPath.filename().string();
    ApplyToPreview(context);
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
    case Section::ServerSettings:
        DrawSettings(context, m_ServerSettings, "Server");
        break;
    }
    ImGui::EndChild();
    EndWindow();
}

} // namespace Hockey
