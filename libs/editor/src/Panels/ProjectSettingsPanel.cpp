#include "Hockey/Editor/Panels/ProjectSettingsPanel.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

#include <imgui.h>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Audio/AudioEvents.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/RuntimeConfigDefaults.hpp"
#include "Hockey/Editor/AssetDragDrop.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorAudioPreview.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

enum class SettingValueKind {
    Boolean,
    Integer,
    Float,
    Enum,
    Text,
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
    return (label != nullptr && label[0] != '\0') ? label : "This setting";
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
        return "Turn " + name + " on to enable that behavior for the current editor/runtime scope; turn it off to disable it. The recommended reset state is " + recommendedValue;
    case SettingValueKind::Enum:
        return name + " uses " + recommendedValue + " as the reset baseline. Lower-cost choices usually reduce detail or automation; higher-quality choices make " + name + " more visible, aggressive, or expensive";
    case SettingValueKind::Integer:
    case SettingValueKind::Float:
        if (!range.empty()) {
            return name + " at " + recommendedValue + " is the reset baseline. Moving toward the lower end of " + range + " reduces " + name + "'s reach, count, frequency, or intensity; moving toward the higher end increases it";
        }
        return name + " at " + recommendedValue + " is the reset baseline. Smaller values reduce " + name + "'s effect; larger values increase it";
    case SettingValueKind::Path:
        return "Leave " + name + " at " + recommendedValue + " to use the configured project asset. Override it with a project-relative path when testing a different scene, prefab, or cooked/raw asset root";
    case SettingValueKind::Text:
        return "Leave " + name + " at " + recommendedValue + " for the default project behavior. Override it with a clear project-specific value when this build, window, server, or asset path needs a different label";
    }
    return name + " uses " + recommendedValue + " as the reset baseline";
}

std::string BuildGeneratedPerformanceImpactText(const char* label,
                                                SettingValueKind kind,
                                                const char* description) {
    const std::string name = LabelText(label);
    const std::string search = ToLowerText(name + " " + ((description != nullptr) ? description : ""));
    if (ContainsAny(search, {"render", "resolution", "scale", "vsync", "hdr", "brightness", "field of view",
                             "upscaler", "sharpening", "texture", "anisotropy", "material", "model", "lod",
                             "bloom", "blur", "depth of field", "lens flare", "volumetric", "particle",
                             "anti-aliasing", "tone mapper", "grain", "chromatic", "vignette", "decal",
                             "ice", "reflection", "scratch", "spray", "trail", "jersey", "net", "crowd",
                             "arena", "glass", "light", "shadow", "atlas", "cascade", "pcf", "bias",
                             "occlusion", "gpu", "fps", "frame time"})) {
        return name + " can affect GPU frame time, memory, or bandwidth. Higher quality, larger budgets, larger atlases, or more visible effects usually cost more; lower values usually improve frame time at a visual tradeoff";
    }
    if (ContainsAny(search, {"physics", "body", "bodies", "collision", "contact", "constraint", "allocator",
                             "thread", "gravity", "substep", "deterministic", "sleeping", "world min"})) {
        return name + " can affect physics CPU time, memory budgets, or simulation stability. Smaller timesteps and higher counts improve capacity or precision but cost more; lower budgets are cheaper but can reject busy scenes";
    }
    if (ContainsAny(search, {"gameplay", "player", "skater", "goalie", "period", "clock", "faceoff", "goal",
                             "puck", "waypoint", "authoritative", "log"})) {
        return name + " affects gameplay simulation, validation, or logging. Higher counts, faster tick rates, wider radii, and debug/log output can add CPU or log work; conservative values keep preview/server cost predictable";
    }
    if (ContainsAny(search, {"asset", "prefab", "path", "cook", "import", "discover", "reload", "audio", "cue",
                             "scene", "startup"})) {
        return name + " can affect asset discovery, import/cook work, disk access, or reload behavior. Automatic processing improves iteration speed but can add background work and metadata churn";
    }
    if (ContainsAny(search, {"server", "tick", "network", "port", "max players", "sleep"})) {
        return name + " can affect server CPU, idle behavior, or future network budget. Higher tick rates and player limits cost more; idle sleeping and conservative defaults reduce local development load";
    }
    if (ContainsAny(search, {"autosave", "validate", "grid", "snap", "camera", "window", "monitor", "input",
                             "mouse", "gamepad"})) {
        return name + " mainly affects editor workflow. Shorter intervals, extra validation, or more overlays can add small CPU/disk/UI work; camera, window, and input values are mostly usability choices";
    }
    if (kind == SettingValueKind::Boolean) {
        return name + " costs only when the enabled behavior does work. Turn it off to remove that behavior from the current workflow";
    }
    return name + " has setting-specific cost based on the system it controls; larger, faster, more frequent, or more detailed values usually cost more than conservative values";
}

std::string BuildSettingTooltip(const char* label, SettingValueKind kind, const char* description, const std::string& recommendedValue,
                                const std::string& range = {}, const char* examples = nullptr,
                                const char* performanceImpact = nullptr) {
    std::string text = (description != nullptr && description[0] != '\0') ? description
                                                                           : "Adjusts this project setting.";
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

template <typename T, std::size_t Count>
bool DrawEnumCombo(const char* label, T& value, const T defaultValue, const std::array<T, Count>& values,
                   const char* tooltip = nullptr, const char* examples = nullptr,
                   const char* performanceImpact = nullptr) {
    bool changed = false;
    const bool open = ImGui::BeginCombo(label, ToString(value));
    ShowSettingTooltip(label, SettingValueKind::Enum, tooltip, ToString(defaultValue), {}, examples, performanceImpact);
    if (open) {
        for (const T option : values) {
            const bool selected = value == option;
            if (ImGui::Selectable(ToString(option), selected)) {
                value = option;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (DrawResetSettingButton(label)) {
        value = defaultValue;
        changed = true;
    }
    return changed;
}

bool DrawUInt(const char* label, std::uint32_t& value, std::uint32_t defaultValue, int minValue, int maxValue,
              const char* tooltip = nullptr, const char* examples = nullptr,
              const char* performanceImpact = nullptr) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    const bool edited = ImGui::DragInt(label, &temp, 1.0f, minValue, maxValue);
    ShowSettingTooltip(label, SettingValueKind::Integer, tooltip, std::to_string(defaultValue),
                       BuildRangeText(minValue, maxValue), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = static_cast<std::uint32_t>(std::clamp(static_cast<int>(defaultValue), minValue, maxValue));
        return true;
    }
    if (edited) {
        value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawInt(const char* label, int& value, int defaultValue, int minValue, int maxValue,
             const char* tooltip = nullptr, const char* examples = nullptr,
             const char* performanceImpact = nullptr) {
    const bool edited = ImGui::DragInt(label, &value, 1.0f, minValue, maxValue);
    ShowSettingTooltip(label, SettingValueKind::Integer, tooltip, std::to_string(defaultValue),
                       BuildRangeText(minValue, maxValue), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = std::clamp(defaultValue, minValue, maxValue);
        return true;
    }
    if (edited) {
        value = std::clamp(value, minValue, maxValue);
        return true;
    }
    return false;
}

bool DrawFloat(const char* label, float& value, float defaultValue, float speed, float minValue, float maxValue,
               const char* format, const char* tooltip = nullptr, const char* examples = nullptr,
               const char* performanceImpact = nullptr) {
    const bool changed = ImGui::DragFloat(label, &value, speed, minValue, maxValue, format);
    ShowSettingTooltip(label, SettingValueKind::Float, tooltip, FormatFloat(defaultValue, format),
                       BuildRangeText(minValue, maxValue, format), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = std::clamp(defaultValue, minValue, maxValue);
        return true;
    }
    return changed;
}

bool DrawCheckbox(const char* label, bool& value, bool defaultValue, const char* tooltip = nullptr,
                  const char* examples = nullptr, const char* performanceImpact = nullptr) {
    const bool changed = ImGui::Checkbox(label, &value);
    ShowSettingTooltip(label, SettingValueKind::Boolean, tooltip, FormatBool(defaultValue), {}, examples,
                       performanceImpact);
    if (DrawResetSettingButton(label)) {
        value = defaultValue;
        return true;
    }
    return changed;
}

bool DrawConfigBool(Config& config, const Config& defaults, const char* key, const char* label, bool fallback,
                    const char* tooltip = nullptr, const char* examples = nullptr,
                    const char* performanceImpact = nullptr) {
    const bool defaultValue = defaults.GetBool(key, fallback);
    bool value = config.GetBool(key, defaultValue);
    const bool edited = ImGui::Checkbox(label, &value);
    ShowSettingTooltip(label, SettingValueKind::Boolean, tooltip, FormatBool(defaultValue), {}, examples,
                       performanceImpact);
    if (DrawResetSettingButton(label)) {
        config.SetBool(key, defaultValue);
        return true;
    }
    if (edited) {
        config.SetBool(key, value);
        return true;
    }
    return false;
}

bool DrawConfigInt(Config& config, const Config& defaults, const char* key, const char* label, int fallback,
                   int minValue, int maxValue, const char* tooltip = nullptr,
                   const char* examples = nullptr, const char* performanceImpact = nullptr) {
    const int defaultValue = defaults.GetInt(key, fallback);
    int value = config.GetInt(key, defaultValue);
    const bool edited = ImGui::DragInt(label, &value, 1.0f, minValue, maxValue);
    ShowSettingTooltip(label, SettingValueKind::Integer, tooltip, std::to_string(defaultValue),
                       BuildRangeText(minValue, maxValue), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        config.SetInt(key, std::clamp(defaultValue, minValue, maxValue));
        return true;
    }
    if (edited) {
        config.SetInt(key, std::clamp(value, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawConfigFloat(Config& config, const Config& defaults, const char* key, const char* label, float fallback,
                     float minValue, float maxValue, const char* tooltip = nullptr,
                     const char* examples = nullptr, const char* performanceImpact = nullptr) {
    const float defaultValue = static_cast<float>(defaults.GetDouble(key, fallback));
    float value = static_cast<float>(config.GetDouble(key, defaultValue));
    const bool edited = ImGui::DragFloat(label, &value, 0.05f, minValue, maxValue, "%.3f");
    ShowSettingTooltip(label, SettingValueKind::Float, tooltip, FormatFloat(defaultValue, "%.3f"),
                       BuildRangeText(minValue, maxValue, "%.3f"), examples, performanceImpact);
    if (DrawResetSettingButton(label)) {
        config.SetDouble(key, std::clamp(defaultValue, minValue, maxValue));
        return true;
    }
    if (edited) {
        config.SetDouble(key, value);
        return true;
    }
    return false;
}

bool DrawConfigString(Config& config, const Config& defaults, const char* key, const char* label, const char* fallback,
                      const char* tooltip = nullptr, const char* examples = nullptr,
                      const char* performanceImpact = nullptr) {
    const std::string defaultValue = defaults.GetString(key, fallback);
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", config.GetString(key, defaultValue).c_str());
    const bool edited = ImGui::InputText(label, buffer, sizeof(buffer));
    ShowSettingTooltip(label, SettingValueKind::Text, tooltip, FormatStringDefault(defaultValue), {}, examples,
                       performanceImpact);
    if (DrawResetSettingButton(label)) {
        config.SetString(key, defaultValue);
        return true;
    }
    if (edited) {
        config.SetString(key, buffer);
        return true;
    }
    return false;
}

bool DrawPrefabPath(const char* label, std::filesystem::path& path, const std::filesystem::path& defaultPath,
                    const char* tooltip = nullptr, const char* examples = nullptr,
                    const char* performanceImpact = nullptr) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", path.generic_string().c_str());

    bool changed = false;
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        path = buffer;
        changed = true;
    }
    ShowSettingTooltip(label, SettingValueKind::Path, tooltip, FormatStringDefault(defaultPath.generic_string()), {},
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
    EditorTooltip::ForLastItem("Clears the waypoint prefab so gameplay keeps click-to-move behavior but stops spawning "
                               "visible waypoint marker instances. Use Reset to restore the recommended marker prefab.");
    if (DrawResetSettingButton(label)) {
        path = defaultPath;
        changed = true;
    }

    return changed;
}

const AssetMetadata* FindAudioAssetByRawPath(AssetManager* assetManager, const std::string& rawPath) {
    if (assetManager == nullptr || rawPath.empty()) {
        return nullptr;
    }
    const AssetMetadata* metadata = assetManager->Database().FindByRawPath(rawPath);
    return metadata != nullptr && metadata->type == AssetType::Audio ? metadata : nullptr;
}

const AssetMetadata* FindAudioAssetById(AssetManager* assetManager, AssetID id) {
    if (assetManager == nullptr || !id.IsValid()) {
        return nullptr;
    }
    const AssetMetadata* metadata = assetManager->Database().Find(id);
    return metadata != nullptr && metadata->type == AssetType::Audio ? metadata : nullptr;
}

std::string AudioCueAssetLabel(const AssetMetadata* metadata, const std::string& rawPath) {
    if (metadata != nullptr) {
        const std::string path = metadata->rawPath.generic_string();
        return metadata->name.empty() ? path : metadata->name + " (" + path + ")";
    }
    return rawPath.empty() ? std::string("None") : rawPath + " (missing)";
}

void ApplyEditorAudioCueMap(EditorContext& context, const Config& config) {
    if (context.audioPreview != nullptr) {
        context.audioPreview->SetCueMap(ResolveHockeyAudioCueMap(config, context.assetManager));
    }
}

bool DrawAudioCueRow(Config& config, const AudioCueConfigBinding& binding, AssetManager* assetManager,
                     EditorAudioPreview* audioPreview) {
    bool changed = false;
    const std::string rawPath = HockeyAudioCueRawPath(config, binding);
    const AssetMetadata* current = FindAudioAssetByRawPath(assetManager, rawPath);
    const std::string currentLabel = AudioCueAssetLabel(current, rawPath);

    ImGui::PushID(binding.key);
    ImGui::TextUnformatted(binding.displayName);
    if (ImGui::BeginCombo("Audio asset", currentLabel.c_str())) {
        if (assetManager != nullptr) {
            std::vector<AssetMetadata*> audioAssets = assetManager->Database().FindByType(AssetType::Audio);
            std::sort(audioAssets.begin(), audioAssets.end(), [](const AssetMetadata* lhs, const AssetMetadata* rhs) {
                return lhs->rawPath.generic_string() < rhs->rawPath.generic_string();
            });
            for (const AssetMetadata* asset : audioAssets) {
                const std::string label = AudioCueAssetLabel(asset, asset->rawPath.generic_string());
                const bool selected = current != nullptr && current->id == asset->id;
                if (ImGui::Selectable(label.c_str(), selected)) {
                    config.SetString(binding.key, asset->rawPath.generic_string());
                    changed = true;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        } else {
            ImGui::TextDisabled("No asset database");
        }
        ImGui::EndCombo();
    }
    EditorTooltip::ForLastItem("Selects the audio asset used when this cue is triggered.");

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropType)) {
            if (payload->DataSize == sizeof(AssetDragPayload)) {
                const AssetDragPayload* assetPayload = static_cast<const AssetDragPayload*>(payload->Data);
                if (static_cast<AssetType>(assetPayload->type) == AssetType::Audio) {
                    if (const AssetMetadata* dropped = FindAudioAssetById(assetManager, AssetID(assetPayload->id))) {
                        config.SetString(binding.key, dropped->rawPath.generic_string());
                        changed = true;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    char pathBuffer[512];
    std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", rawPath.c_str());
    if (ImGui::InputText("Path", pathBuffer, sizeof(pathBuffer))) {
        config.SetString(binding.key, pathBuffer);
        changed = true;
    }
    EditorTooltip::ForLastItem("Project-relative raw audio path used for this cue.");

    const bool canPreview = current != nullptr && audioPreview != nullptr && audioPreview->IsReady();
    ImGui::BeginDisabled(!canPreview);
    if (ImGui::SmallButton("Preview") && current != nullptr && audioPreview != nullptr) {
        audioPreview->Preview(current->id);
    }
    ImGui::EndDisabled();
    EditorTooltip::ForLastItem("Plays the currently assigned audio asset.");

    if (DrawResetSettingButton("Audio asset")) {
        config.SetString(binding.key, binding.defaultRawPath);
        changed = true;
    }
    ImGui::Spacing();
    ImGui::PopID();
    return changed;
}

void ResetConfigBool(Config& config, const Config& defaults, const char* key, bool fallback) {
    config.SetBool(key, defaults.GetBool(key, fallback));
}

void ResetConfigInt(Config& config, const Config& defaults, const char* key, int fallback) {
    config.SetInt(key, defaults.GetInt(key, fallback));
}

void ResetConfigFloat(Config& config, const Config& defaults, const char* key, float fallback) {
    config.SetDouble(key, defaults.GetDouble(key, fallback));
}

void ResetConfigString(Config& config, const Config& defaults, const char* key, const char* fallback) {
    config.SetString(key, defaults.GetString(key, fallback));
}

void DrawRestartBadge(bool restartRequired, const char* text) {
    if (restartRequired) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", text);
    }
}

bool DrawRendererSettings(RendererSettings& settings, const RendererSettings& defaults) {
    bool changed = false;
    bool custom = false;

    constexpr std::array kDisplayModes = {DisplayMode::Windowed, DisplayMode::BorderlessFullscreen,
                                          DisplayMode::ExclusiveFullscreen};
    constexpr std::array kPresets = {GraphicsPreset::Low, GraphicsPreset::Medium, GraphicsPreset::High,
                                     GraphicsPreset::Ultra, GraphicsPreset::Custom};
    constexpr std::array kUpscalers = {Upscaler::None, Upscaler::FSR, Upscaler::XeSS, Upscaler::DLSS};
    constexpr std::array kTextureQualities = {TextureQuality::Low, TextureQuality::Medium, TextureQuality::High,
                                              TextureQuality::Ultra};
    constexpr std::array kReflectionQualities = {ReflectionQuality::Off, ReflectionQuality::Low,
                                                 ReflectionQuality::Medium, ReflectionQuality::High,
                                                 ReflectionQuality::Ultra};
    constexpr std::array kDetailQualities = {DetailQuality::Low, DetailQuality::Medium, DetailQuality::High,
                                             DetailQuality::Ultra};
    constexpr std::array kEffectQualities = {EffectQuality::Off, EffectQuality::Low, EffectQuality::Medium,
                                             EffectQuality::High, EffectQuality::Ultra};
    constexpr std::array kAntiAliasing = {AntiAliasing::Off, AntiAliasing::FXAA, AntiAliasing::TAA,
                                          AntiAliasing::MSAA2x, AntiAliasing::MSAA4x, AntiAliasing::MSAA8x};
    constexpr std::array kToneMappers = {ToneMapper::Linear, ToneMapper::Reinhard, ToneMapper::ACES};

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawEnumCombo("Display mode", settings.displayMode, defaults.displayMode, kDisplayModes,
                                "Controls whether the scene is viewed in a window, borderless desktop surface, or "
                                "exclusive fullscreen output. Windowed is easiest for editor iteration; fullscreen "
                                "modes are mainly for packaged client presentation testing.");
        custom |= DrawUInt("Resolution width", settings.resolutionWidth, defaults.resolutionWidth, 320, 7680,
                           "Backbuffer width in pixels. Higher values make the scene sharper horizontally and cost "
                           "more GPU memory and fill rate.");
        custom |= DrawUInt("Resolution height", settings.resolutionHeight, defaults.resolutionHeight, 240, 4320,
                           "Backbuffer height in pixels. Higher values make the scene sharper vertically and cost "
                           "more GPU memory and fill rate.");
        custom |= DrawUInt("Refresh rate", settings.refreshRate, defaults.refreshRate, 0, 480,
                           "Preferred display refresh rate for scene presentation. Use 0 to accept the monitor "
                           "default; choose a fixed rate only when validating frame pacing on a specific display.");
        custom |= DrawUInt("Monitor index", settings.monitorIndex, defaults.monitorIndex, 0, 16,
                           "Selects which monitor shows the scene when fullscreen modes are used.");
        custom |= DrawCheckbox("VSync", settings.vsync, defaults.vsync,
                               "Synchronizes scene presentation to display refresh to reduce tearing, usually with "
                               "more input latency. Disable it when profiling raw renderer throughput or input "
                               "latency.");
        custom |= DrawUInt("FPS limit", settings.fpsLimit, defaults.fpsLimit, 0, 1000,
                           "Caps how often the scene is rendered. Use 0 for no renderer-side cap, or set a fixed "
                           "limit when comparing visual settings at a stable frame budget.");
        custom |= DrawCheckbox("HDR", settings.hdr, defaults.hdr,
                               "Requests HDR scene rendering and presentation so bright ice highlights and arena "
                               "lights keep more range when supported.");
        custom |= DrawFloat("Brightness", settings.brightness, defaults.brightness, 0.01f, 0.1f, 4.0f, "%.2f",
                            "Scene brightness multiplier. Higher values lift the whole rendered scene; lower values "
                            "make lighting appear darker.");
        custom |= DrawFloat("Field of view", settings.fieldOfView, defaults.fieldOfView, 0.1f, 1.0f, 179.0f, "%.1f",
                            "Default camera vertical field of view in degrees. Wider values show more of the rink "
                            "but make scene objects look smaller.");
    }

    if (ImGui::CollapsingHeader("Scaling", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawFloat("Render scale", settings.renderScale, defaults.renderScale, 0.01f, 0.25f, 2.0f, "%.2f",
                             "Internal render resolution multiplier. Changing render scale changes how sharp the scene appears before upscaling; lower values improve performance and soften image detail, while values above 1.0 improve clarity at a steep GPU cost.",
                             "0.75 reduces GPU cost but softens the image; 1.0 renders at native resolution; 1.25 or 2.0 increases sharpness but scales fill-rate cost quickly.",
                             "Render scale directly changes GPU shading and bandwidth work because pixel count scales in both dimensions.");
        custom |= DrawCheckbox("Dynamic resolution", settings.dynamicResolution, defaults.dynamicResolution,
                               "Allows the renderer to lower or raise scene resolution at runtime to protect frame "
                               "rate during heavy lighting or crowd shots. Keep this off for deterministic visual "
                               "comparison screenshots.");
        custom |= DrawEnumCombo("Upscaler", settings.upscaler, defaults.upscaler, kUpscalers,
                                "Selects the reconstruction method used when the scene is rendered below output "
                                "resolution. None keeps native rendering; FSR, XeSS, and DLSS are persisted targets "
                                "for renderer integrations that can reconstruct lower render scales.");
        custom |= DrawFloat("Sharpening", settings.sharpening, defaults.sharpening, 0.01f, 0.0f, 2.0f, "%.2f",
                            "Post-upscale sharpening strength. Higher values make boards, jersey numbers, and ice "
                            "scratches crisper but can add ringing.");
    }

    if (ImGui::CollapsingHeader("Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        GraphicsPreset preset = settings.preset;
        if (DrawEnumCombo("Preset", preset, defaults.preset, kPresets,
                          "Applies a bundled graphics quality baseline that changes scene detail, shadows, effects, "
                          "and performance together.")) {
            settings = ApplyGraphicsPreset(preset, settings);
            changed = true;
        }
        custom |= DrawEnumCombo("Texture quality", settings.textureQuality, defaults.textureQuality, kTextureQualities,
                                "Controls texture mip detail in the scene. Higher quality keeps jerseys, boards, "
                                "and ice marks sharper at distance.");
        custom |= DrawUInt("Texture budget MB", settings.textureStreamingBudgetMB, defaults.textureStreamingBudgetMB, 128, 32768,
                           "Approximate texture streaming memory budget. Raising it lets more high-detail scene "
                           "textures stay resident.");
        custom |= DrawUInt("Anisotropy", settings.anisotropy, defaults.anisotropy, 1, 16,
                           "Maximum anisotropic filtering level. Higher values sharpen ice, boards, and ads viewed "
                           "at glancing angles.");
        custom |= DrawEnumCombo("Material quality", settings.materialQuality, defaults.materialQuality, kDetailQualities,
                                "Controls material feature and shader quality for scene surfaces such as ice, glass, "
                                "jerseys, and metal.");
        custom |= DrawEnumCombo("Model quality", settings.modelQuality, defaults.modelQuality, kDetailQualities,
                                "Controls mesh detail targets for scene geometry. Higher quality keeps models more "
                                "detailed farther from the camera.");
        custom |= DrawFloat("LOD distance multiplier", settings.lodDistanceMultiplier, defaults.lodDistanceMultiplier, 0.01f, 0.1f, 10.0f, "%.2f",
                            "Scales model LOD switch distances. Higher values keep detailed scene meshes visible "
                            "farther away at a performance cost.");
    }

    if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawCheckbox("Bloom", settings.bloom, defaults.bloom,
                               "Adds glow around bright scene highlights such as arena lights, ice glare, and "
                               "emissive signage.");
        custom |= DrawCheckbox("Motion blur", settings.motionBlur, defaults.motionBlur,
                               "Blurs fast camera or object movement in the scene, making skating and puck motion "
                               "feel smoother but less crisp.");
        custom |= DrawCheckbox("Depth of field", settings.depthOfField, defaults.depthOfField,
                               "Adds focus-based background blur so distant scene objects can soften around the "
                               "active camera focus.");
        custom |= DrawCheckbox("Lens flare", settings.lensFlare, defaults.lensFlare,
                               "Adds flare artifacts from bright scene lights and reflections.");
        custom |= DrawEnumCombo("Volumetric lighting", settings.volumetricLighting, defaults.volumetricLighting,
                                kDetailQualities,
                                "Controls fog and light-volume quality. Higher values make arena beams and haze "
                                "smoother.");
        custom |= DrawEnumCombo("Particle quality", settings.particleQuality, defaults.particleQuality,
                                kDetailQualities,
                                "Controls scene particle detail and count, including skate spray and impact effects.");
        custom |= DrawEnumCombo("Anti-aliasing", settings.antiAliasing, defaults.antiAliasing, kAntiAliasing,
                                "Selects edge smoothing. FXAA is the implemented default and reduces shimmer on "
                                "boards, sticks, nets, and skaters with low cost. TAA and MSAA choices are persisted "
                                "for future renderer paths.");
        custom |= DrawEnumCombo("Tone mapper", settings.toneMapper, defaults.toneMapper, kToneMappers,
                                "Chooses how HDR scene lighting is mapped to display colors, affecting contrast and "
                                "highlight rolloff.");
        custom |= DrawCheckbox("Film grain", settings.filmGrain, defaults.filmGrain,
                               "Adds a fine grain over the scene for style; disable for the cleanest image.");
        custom |= DrawCheckbox("Chromatic aberration", settings.chromaticAberration, defaults.chromaticAberration,
                               "Adds color fringing near scene image edges; disable for sharper inspection.");
        custom |= DrawCheckbox("Vignette", settings.vignette, defaults.vignette,
                               "Darkens scene image corners to draw attention toward the rink center.");
    }

    if (ImGui::CollapsingHeader("Decals", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawCheckbox("Decals", settings.decals, defaults.decals,
                               "Enables forward-rendered decals for ice marks, ads, scuffs, and gameplay effects.");
        custom |= DrawUInt("Max rendered decals", settings.maxRenderedDecals, defaults.maxRenderedDecals, 0,
                           kRendererMaxDecals, "Caps how many decals are submitted for the scene in one frame.");
    }

    if (ImGui::CollapsingHeader("Hockey", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawEnumCombo("Ice quality", settings.iceQuality, defaults.iceQuality, kDetailQualities,
                                "Controls the ice surface shader and detail. Higher values improve gloss, markings, "
                                "and worn-surface response to lights once the matching polish renderer paths exist; "
                                "today this is a persisted quality target for authored defaults.");
        custom |= DrawEnumCombo("Ice reflection", settings.iceReflectionQuality, defaults.iceReflectionQuality,
                                kReflectionQualities,
                                "Ice reflection changes how clearly lights, players, and boards reflect on the rink "
                                "surface. These options are persisted quality targets until the matching polish renderer paths exist.");
        custom |= DrawEnumCombo("Ice scratches", settings.iceScratchQuality, defaults.iceScratchQuality,
                                kDetailQualities,
                                "Controls scratch and wear detail on the ice so skating paths and surface damage read "
                                "more clearly. Higher values should add visible wear detail once the polish material path is wired.");
        custom |= DrawEnumCombo("Skate spray", settings.skateSprayQuality, defaults.skateSprayQuality,
                                kEffectQualities,
                                "Controls skate spray particles around hard turns, stops, and collisions. This is a "
                                "persisted quality target until the matching polish renderer paths exist.");
        custom |= DrawEnumCombo("Puck trail", settings.puckTrailQuality, defaults.puckTrailQuality, kEffectQualities,
                                "Controls puck trail visibility and detail so fast shots are easier to track. This is "
                                "a persisted quality target until the matching polish renderer paths exist.");
        custom |= DrawEnumCombo("Jersey quality", settings.jerseyQuality, defaults.jerseyQuality, kDetailQualities,
                                "Controls jersey material and mesh detail for player readability in the scene.");
        custom |= DrawEnumCombo("Goal net quality", settings.goalNetQuality, defaults.goalNetQuality,
                                kDetailQualities,
                                "Controls goal net mesh and material detail, affecting how netting reads near the "
                                "crease.");
        custom |= DrawEnumCombo("Crowd quality", settings.crowdQuality, defaults.crowdQuality, kDetailQualities,
                                "Controls crowd rendering detail in arena backgrounds.");
        custom |= DrawEnumCombo("Arena detail", settings.arenaDetail, defaults.arenaDetail, kDetailQualities,
                                "Controls prop and environment detail around the rink.");
        custom |= DrawEnumCombo("Board glass detail", settings.boardGlassDetail, defaults.boardGlassDetail,
                                kDetailQualities,
                                "Controls board-glass material and reflection detail around the play surface.");
    }

    if (ImGui::CollapsingHeader("Debug Overlays", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawCheckbox("Show FPS", settings.showFPS, defaults.showFPS,
                               "Displays frames per second over the scene while tuning visual settings.");
        custom |= DrawCheckbox("Show frame time", settings.showFrameTime, defaults.showFrameTime,
                               "Displays frame timing over the scene to spot costly lighting, shadows, or effects.");
        custom |= DrawCheckbox("Show GPU stats", settings.showGPUStats, defaults.showGPUStats,
                               "Displays renderer GPU statistics over the scene for graphics tuning.");
        custom |= DrawCheckbox("Show network stats", settings.showNetworkStats, defaults.showNetworkStats,
                               "Displays network statistics over the client scene when network overlays are active.");
    }

    if (custom) {
        settings.preset = GraphicsPreset::Custom;
    }
    return changed || custom;
}

bool DrawLightingShadowSettings(RendererSettings& settings, const RendererSettings& defaults) {
    bool changed = false;

    if (ImGui::CollapsingHeader("Budgets", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Budgets");
        changed |= DrawUInt("Max rendered lights", settings.maxRenderedLights, defaults.maxRenderedLights, 0, 16,
                            "Caps how many scene lights the renderer submits. Lower values improve performance but "
                            "may leave lower-priority lights unrendered.");
        changed |= DrawUInt("Max local shadow tiles", settings.maxLocalShadowTiles, defaults.maxLocalShadowTiles, 0, 16,
                            "Caps shadow atlas tiles for point and spot lights. More tiles let more local scene "
                            "lights cast shadows at the same time.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Shadow Atlases", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Shadow Atlases");
        changed |= DrawEnumCombo("Shadow quality", settings.shadowQuality, defaults.shadowQuality,
                                 std::array{ShadowQuality::Off, ShadowQuality::Low, ShadowQuality::Medium,
                                            ShadowQuality::High, ShadowQuality::Ultra},
                                 "Overall shadow quality preset. Higher settings improve scene shadow sharpness, "
                                 "distance, and filtering at a GPU cost.");
        changed |= DrawFloat("Shadow distance", settings.shadowDistance, defaults.shadowDistance, 1.0f, 0.0f,
                             5000.0f, "%.0f",
                             "Maximum distance for shadow rendering. Raising it lets rink, player, and arena shadows "
                             "remain visible farther from the camera.");
        changed |= DrawUInt("Directional atlas", settings.directionalShadowAtlasResolution,
                            defaults.directionalShadowAtlasResolution, 256, 16384,
                            "Directional shadow atlas resolution. Directional atlas controls how much texel detail the sun shadow map has; "
                            "the default is the concrete atlas size chosen by the active quality preset.",
                            "2048 saves memory but softens distant sun shadows; 4096 is the balanced default; 8192 consumes substantially more shadow memory while sharpening large-rink directional shadows.",
                            "Atlas memory grows with width times height, so doubling resolution roughly quadruples shadow-map memory and bandwidth.");
        changed |= DrawUInt("Local atlas", settings.localShadowAtlasResolution, defaults.localShadowAtlasResolution,
                            256, 16384,
                            "Local light shadow atlas resolution. Higher values sharpen point and spot light shadows "
                            "from arena fixtures; the default is the concrete atlas size chosen by the active quality "
                            "preset.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Directional Cascades", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Directional Cascades");
        changed |= DrawUInt("Cascade count", settings.shadowCascadeCount, defaults.shadowCascadeCount, 1, 4,
                            "Cascade count controls how far directional shadows stay detailed across the camera view; "
                            "the default is the concrete cascade count chosen by the active quality preset.");
        changed |= DrawFloat("Split lambda", settings.shadowCascadeSplitLambda, defaults.shadowCascadeSplitLambda,
                             0.01f, 0.0f, 1.0f, "%.2f",
                             "Controls where cascade resolution is spent. Higher values reserve more directional "
                             "shadow detail near the camera.");
        changed |= DrawFloat("Overlap scale", settings.shadowCascadeOverlapScale, defaults.shadowCascadeOverlapScale,
                             0.01f, 0.0f, 1.0f, "%.2f",
                             "Scales overlap between neighboring cascades to reduce visible shadow seams while the "
                             "camera moves through the scene.");
        changed |= DrawFloat("Minimum overlap", settings.shadowCascadeMinOverlap, defaults.shadowCascadeMinOverlap,
                             0.25f, 0.0f, 100.0f, "%.2f",
                             "Minimum cascade overlap distance. Higher values make cascade transitions safer for "
                             "large open rink views.");
        changed |= DrawFloat("Max overlap scale", settings.shadowCascadeMaxOverlapScale,
                             defaults.shadowCascadeMaxOverlapScale, 0.01f, 0.0f, 1.0f, "%.2f",
                             "Upper bound for cascade overlap scaling so distant scene shadows do not spend "
                                      "too much range on transitions.");
        changed |= DrawFloat("Blend scale", settings.shadowCascadeBlendScale, defaults.shadowCascadeBlendScale,
                             0.01f, 0.0f, 1.0f, "%.2f",
                             "Scales the soft blend region between cascades. Higher values hide cascade transitions "
                             "but soften shadows across the blend.");
        changed |= DrawFloat("Minimum blend", settings.shadowCascadeMinBlend, defaults.shadowCascadeMinBlend,
                             0.05f, 0.0f, 50.0f, "%.2f",
                             "Minimum distance used for cascade blending so nearby scene shadows do not switch "
                             "abruptly.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Directional Filter & Bias", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Directional Filter & Bias");
        changed |= DrawUInt("PCF radius", settings.directionalShadowPcfRadius, defaults.directionalShadowPcfRadius, 0, 3,
                            "Directional shadow filter radius. Higher values soften sun shadows from players, sticks, "
                            "and rink props.");
        changed |= DrawFloat("Normal offset scale", settings.directionalShadowNormalOffsetScale,
                             defaults.directionalShadowNormalOffsetScale, 0.01f, 0.0f, 4.0f, "%.2f",
                             "Scales normal-based directional shadow offset to reduce acne on lit "
                                             "scene surfaces.");
        changed |= DrawFloat("Normal offset min", settings.directionalShadowNormalOffsetMin,
                             defaults.directionalShadowNormalOffsetMin, 0.0005f, 0.0f, 0.25f, "%.4f",
                             "Minimum normal offset for directional shadows so flat surfaces like ice "
                                             "avoid self-shadow speckling.");
        changed |= DrawFloat("Normal offset max", settings.directionalShadowNormalOffsetMax,
                             defaults.directionalShadowNormalOffsetMax, 0.001f, 0.0f, 1.0f, "%.4f",
                             "Maximum normal offset for directional shadows. Too high can detach shadows from "
                                      "skaters and boards.");
        changed |= DrawFloat("Bias base", settings.directionalShadowBiasBase, defaults.directionalShadowBiasBase,
                             0.00005f, 0.0f, 0.02f, "%.5f",
                             "Base depth bias for directional shadows. Raising it reduces acne but can make shadows "
                             "float away from scene objects.");
        changed |= DrawFloat("Bias slope", settings.directionalShadowBiasSlope, defaults.directionalShadowBiasSlope,
                             0.00005f, 0.0f, 0.05f, "%.5f",
                             "Slope-scaled depth bias for directional shadows on angled scene surfaces.");
        changed |= DrawFloat("Bias min", settings.directionalShadowBiasMin, defaults.directionalShadowBiasMin,
                             0.00005f, 0.0f, 0.02f, "%.5f",
                             "Minimum computed directional shadow bias applied to keep close-up shadows stable.");
        changed |= DrawFloat("Bias max", settings.directionalShadowBiasMax, defaults.directionalShadowBiasMax,
                             0.00005f, 0.0f, 0.05f, "%.5f",
                             "Maximum computed directional shadow bias. Lower values keep contact tighter but risk "
                             "shadow acne.");
        changed |= DrawFloat("Depth bias constant", settings.directionalShadowDepthBiasConstant,
                             defaults.directionalShadowDepthBiasConstant, 0.05f, 0.0f, 8.0f, "%.2f",
                             "Rasterizer constant depth bias for directional shadows during shadow-map "
                                             "rendering.");
        changed |= DrawFloat("Depth bias slope", settings.directionalShadowDepthBiasSlope,
                             defaults.directionalShadowDepthBiasSlope, 0.05f, 0.0f, 8.0f, "%.2f",
                             "Rasterizer slope depth bias for directional shadow casters on angled geometry.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Contact Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Contact Shadows");
        changed |= DrawCheckbox("Contact shadows", settings.contactShadows, defaults.contactShadows,
                               "Contact shadows add small grounding shadows under skates, puck, sticks, and props "
                               "where shadow maps are too coarse.");
        changed |= DrawFloat("Distance", settings.contactShadowDistance, defaults.contactShadowDistance, 0.5f, 0.0f,
                             500.0f, "%.1f",
                             "Maximum contact shadow ray distance. Higher values extend fine grounding shadows around "
                             "nearby scene objects.");
        changed |= DrawFloat("Strength", settings.contactShadowStrength, defaults.contactShadowStrength, 0.01f, 0.0f,
                             1.0f, "%.2f",
                             "Contact shadow opacity multiplier. Higher values make contact points darker and more "
                             "anchored.");
        changed |= DrawFloat("Normal offset scale", settings.contactShadowNormalOffsetScale,
                             defaults.contactShadowNormalOffsetScale, 0.01f, 0.0f, 2.0f, "%.2f",
                             "Scales normal-based contact shadow offset to avoid self-shadow speckles on "
                                      "close surfaces.");
        changed |= DrawFloat("Normal offset min", settings.contactShadowNormalOffsetMin,
                             defaults.contactShadowNormalOffsetMin, 0.0005f, 0.0f, 0.25f, "%.4f",
                             "Minimum normal offset for contact shadows on flat scene surfaces.");
        changed |= DrawFloat("Bias base", settings.contactShadowBiasBase, defaults.contactShadowBiasBase, 0.00005f,
                             0.0f, 0.02f, "%.5f",
                             "Base depth bias for contact shadows. Raising it reduces artifacts but can weaken tiny "
                             "grounding shadows.");
        changed |= DrawFloat("Bias slope", settings.contactShadowBiasSlope, defaults.contactShadowBiasSlope,
                             0.00005f, 0.0f, 0.05f, "%.5f",
                             "Slope-scaled depth bias for contact shadows on angled geometry.");
        changed |= DrawFloat("Bias min", settings.contactShadowBiasMin, defaults.contactShadowBiasMin, 0.00005f,
                             0.0f, 0.02f, "%.5f",
                             "Minimum computed contact shadow bias used for stable close-up grounding.");
        changed |= DrawFloat("Bias max", settings.contactShadowBiasMax, defaults.contactShadowBiasMax, 0.00005f,
                             0.0f, 0.05f, "%.5f",
                             "Maximum computed contact shadow bias. Lower values keep contacts tight but can add "
                             "speckling.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Local Light Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Local Light Shadows");
        changed |= DrawUInt("PCF radius", settings.localShadowPcfRadius, defaults.localShadowPcfRadius, 0, 3,
                            "Local light shadow filter radius. Higher values soften point and spot shadows from arena "
                            "fixtures.");
        changed |= DrawFloat("Bias scale", settings.localShadowBiasScale, defaults.localShadowBiasScale, 0.00005f,
                             0.0f, 0.05f, "%.5f",
                             "Scales depth bias for local light shadows. Raising it reduces acne but can detach "
                             "shadows from props.");
        changed |= DrawFloat("Bias min", settings.localShadowBiasMin, defaults.localShadowBiasMin, 0.00005f, 0.0f,
                             0.02f, "%.5f",
                             "Minimum local light shadow bias for stable close local shadows.");
        changed |= DrawFloat("Bias max", settings.localShadowBiasMax, defaults.localShadowBiasMax, 0.00005f, 0.0f,
                             0.05f, "%.5f",
                             "Maximum local light shadow bias. Lower values keep point and spot shadows tighter.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Ambient & Reflections", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Ambient & Reflections");
        changed |= DrawEnumCombo("Ambient occlusion", settings.aoQuality, defaults.aoQuality,
                                 std::array{AmbientOcclusionQuality::Off, AmbientOcclusionQuality::Low,
                                            AmbientOcclusionQuality::Medium, AmbientOcclusionQuality::High,
                                            AmbientOcclusionQuality::Ultra},
                                 "Screen-space ambient occlusion quality. Higher values darken creases and contact "
                                 "areas around skates, boards, nets, and props.");
        changed |= DrawEnumCombo("Reflection quality", settings.reflectionQuality, defaults.reflectionQuality,
                                 std::array{ReflectionQuality::Off, ReflectionQuality::Low,
                                            ReflectionQuality::Medium, ReflectionQuality::High,
                                            ReflectionQuality::Ultra},
                                 "Reflection probe and screen reflection quality. Higher values improve reflected "
                                 "arena lights and nearby scene objects.");
        changed |= DrawEnumCombo("Global illumination", settings.globalIlluminationQuality,
                                 defaults.globalIlluminationQuality,
                                 std::array{DetailQuality::Low, DetailQuality::Medium, DetailQuality::High,
                                            DetailQuality::Ultra},
                                 "Indirect lighting quality target. Higher values make bounce light from ice, boards, "
                                 "and arena surfaces more convincing.");
        ImGui::PopID();
    }

    if (changed) {
        settings = ClampRendererSettings(settings);
        settings.preset = GraphicsPreset::Custom;
    }
    return changed;
}

bool DrawPhysicsSettings(PhysicsSettings& settings, const PhysicsSettings& defaults, bool serverMode) {
    bool changed = false;
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, defaults.fixedDeltaSeconds, 0.0005f,
                         0.001f, 0.1f, "%.4f",
                         "Physics fixed delta controls how often preview collisions, skating contacts, and puck "
                         "movement are simulated. Smaller values increase stability and CPU cost; larger values are "
                         "cheaper but make fast puck contacts and stacked bodies less reliable.",
                         "0.0083 runs physics at roughly 120 Hz; 0.0167 is roughly 60 Hz; 0.0333 is roughly 30 Hz and can miss fast contacts.",
                         "Smaller fixed deltas run more physics ticks per second and increase CPU cost; larger fixed deltas reduce CPU cost but lower collision precision.");
    changed |= DrawUInt("Max bodies", settings.maxBodies, defaults.maxBodies, 1, 1000000,
                        "Maximum physics bodies allowed in the scene before new dynamic objects fail to register. "
                        "Raise this only for unusually dense test scenes because higher budgets reserve more memory.");
    changed |= DrawUInt("Max body pairs", settings.maxBodyPairs, defaults.maxBodyPairs, 1, 1000000,
                        "Maximum broadphase body pairs. Higher values support denser scenes with more possible "
                        "collisions.");
    changed |= DrawUInt("Max contact constraints", settings.maxContactConstraints, defaults.maxContactConstraints, 1,
                        1000000,
                        "Maximum active contact constraints. Raise this for scenes with many simultaneous body, puck, "
                        "board, and prop contacts.");
    changed |= DrawUInt("Temp allocator MB", settings.tempAllocatorSizeMB, defaults.tempAllocatorSizeMB, 1, 4096,
                        "Temporary physics memory used while solving scene contacts and simulation islands. Too low "
                        "can cause physics initialization or busy-contact failures; too high wastes memory.");
    changed |= DrawUInt("Job thread count", settings.jobThreadCount, defaults.jobThreadCount, 0, 128,
                        "Physics worker thread count. 0 lets the engine choose based on hardware.");
    changed |= DrawFloat("Gravity X", settings.gravity.x, defaults.gravity.x, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Horizontal gravity applied to dynamic scene bodies. Keep this near zero for a flat hockey "
                         "rink unless testing unusual arena effects.");
    changed |= DrawFloat("Gravity Y", settings.gravity.y, defaults.gravity.y, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Vertical gravity applied to dynamic scene bodies, including airborne puck and prop motion.");
    changed |= DrawFloat("Gravity Z", settings.gravity.z, defaults.gravity.z, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Depth-axis gravity applied to dynamic scene bodies. Keep this near zero for normal rink "
                         "play so loose pucks and props do not drift down-ice unnaturally.");
    changed |= DrawUInt("Collision steps", settings.collisionSteps, defaults.collisionSteps, 1, 32,
                        "Collision solver steps per fixed update. Higher values improve contact stability in busy "
                        "scenes.");
    changed |= DrawUInt("Integration substeps", settings.integrationSubsteps, defaults.integrationSubsteps, 1, 32,
                        "Subdivides each fixed update. Higher values make fast puck and player contacts more stable "
                        "at a CPU cost.");
    if (serverMode) {
        changed |= DrawCheckbox("Deterministic mode", settings.deterministicMode, defaults.deterministicMode,
                                "Keeps authoritative scene simulation reproducible for server snapshots and replay.");
    }
    changed |= DrawCheckbox("Enable sleeping", settings.enableSleeping, defaults.enableSleeping,
                            "Lets inactive scene bodies sleep to save CPU until contact or force wakes them.");
    changed |= DrawCheckbox("Enable debug draw", settings.enableDebugDraw, defaults.enableDebugDraw,
                            "Draws physics shapes and contacts over the scene for collider inspection. Leave it off "
                            "for normal preview because the overlay can obscure art and costs debug draw work.");
    changed |= DrawFloat("World min Y", settings.worldMinY, defaults.worldMinY, 1.0f, -100000.0f, 0.0f, "%.0f",
                         "Vertical kill plane for physics bodies that fall out of the scene. More negative values "
                         "allow long falls before cleanup; values closer to zero catch broken scene setups sooner.");
    return changed;
}

bool DrawGameplaySettings(GameplaySettings& settings, const GameplaySettings& defaults) {
    bool changed = false;
    changed |= DrawCheckbox("Enabled", settings.enabled, defaults.enabled,
                            "Enables hockey gameplay systems for the scene, including puck, teams, scoring, and game "
                            "flow.");
    changed |= DrawUInt("Target player count", settings.targetPlayerCount, defaults.targetPlayerCount, 1, 64,
                        "Target player count controls how many skaters and goalies the scene simulation expects. The "
                        "4v4 design uses 8 total players, matching three skaters plus one goalie per team.");
    changed |= DrawUInt("Skaters per team", settings.skatersPerTeam, defaults.skatersPerTeam, 0, 16,
                        "Number of skater spawn slots expected for each team in gameplay scenes.");
    changed |= DrawUInt("Goalies per team", settings.goaliesPerTeam, defaults.goaliesPerTeam, 0, 4,
                        "Number of goalie spawn slots expected for each team in gameplay scenes.");
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, defaults.fixedDeltaSeconds, 0.0005f,
                         0.001f, 0.1f, "%.4f",
                         "Gameplay fixed delta controls how often rules, possession, scoring, and preview input tick.");
    changed |= DrawFloat("Period length seconds", settings.periodLengthSeconds, defaults.periodLengthSeconds, 1.0f,
                         1.0f, 7200.0f, "%.0f",
                         "Length of each gameplay period in seconds for the scene's match flow. Shorter periods speed "
                         "iteration and arcade pacing; longer periods make matches feel more traditional.");
    changed |= DrawUInt("Period count", settings.periodCount, defaults.periodCount, 1, 12,
                        "Number of periods the scene's match flow plays before final game state.");
    changed |= DrawCheckbox("Stop clock after goal", settings.stopClockAfterGoal, defaults.stopClockAfterGoal,
                            "Stops the game clock after goals so the scene can reset for celebration and faceoff.");
    changed |= DrawCheckbox("Auto faceoff after goal", settings.autoFaceoffAfterGoal, defaults.autoFaceoffAfterGoal,
                            "Automatically returns players and puck to faceoff positions after a goal.");
    changed |= DrawFloat("Post-goal delay seconds", settings.postGoalDelaySeconds, defaults.postGoalDelaySeconds,
                         0.1f, 0.0f, 60.0f, "%.1f",
                         "Delay before the scene resets to faceoff after a goal.");
    changed |= DrawFloat("Faceoff delay seconds", settings.faceoffDelaySeconds, defaults.faceoffDelaySeconds, 0.1f,
                         0.0f, 60.0f, "%.1f",
                         "Delay between faceoff setup and live play, keeping players and puck staged before the drop.");
    changed |= DrawFloat("Goal detection radius", settings.goalDetectionRadius, defaults.goalDetectionRadius, 0.01f,
                         0.0f, 20.0f, "%.2f",
                         "Distance from the goal marker where puck position scoring is accepted. Larger radii make "
                         "goals easier to trigger but can allow questionable scores near posts or behind the net.");
    changed |= DrawCheckbox("Require puck for goal", settings.requirePuckForGoal, defaults.requirePuckForGoal,
                            "Requires goal trigger scoring to come from a puck entity rather than any object.");
    changed |= DrawCheckbox("Allow manual goalie", settings.allowManualGoalie, defaults.allowManualGoalie,
                            "Allows a player-controlled goalie role when the scene has goalie spawns.");
    changed |= DrawCheckbox("Allow out of play", settings.allowOutOfPlay, defaults.allowOutOfPlay,
                            "Allows puck and player state to enter out-of-play handling when leaving legal areas.");
    changed |= DrawPrefabPath("Waypoint prefab", settings.waypointPrefabPath, defaults.waypointPrefabPath,
                              "Prefab instantiated for visible click-to-move waypoint markers. Drag a prefab from "
                              "the Project panel or type a project-relative .prefab.yaml path.");
    changed |= DrawCheckbox("Debug draw gameplay", settings.debugDrawGameplay, defaults.debugDrawGameplay,
                            "Draws gameplay markers, zones, and role debug information over the scene. Use it while "
                            "authoring spawn pools, faceoff spots, and puck interactions; leave it off for clean art review.");
    changed |= DrawCheckbox("Log gameplay events", settings.logGameplayEvents, defaults.logGameplayEvents,
                            "Logs gameplay events from the scene such as goals, faceoffs, possession, and rule "
                            "changes.");
    return changed;
}

} // namespace

ProjectSettingsPanel::ProjectSettingsPanel()
    : Panel(EditorPanelNames::kProjectSettings, /*openByDefault=*/false) {}

void ProjectSettingsPanel::OnImGui(EditorContext& context) {
    EnsureLoaded(context);
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    if (ImGui::Button("Reload")) {
        LoadAll(context);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All Project Settings")) {
        m_EditorConfig = m_DefaultConfig;
        m_ServerConfig = m_DefaultConfig;
        RefreshDerivedSettings();
        SaveEditorConfig(context);
        if (context.renderer != nullptr && context.renderer->IsInitialized()) {
            context.renderer->ApplySettings(m_EditorRenderer);
        }
        if (context.physicsPreview != nullptr) {
            context.physicsPreview->Configure(m_EditorPhysics);
        }
        if (context.gameplayPreview != nullptr) {
            context.gameplayPreview->Configure(m_EditorGameplay);
        }
        ApplyEditorAudioCueMap(context, m_EditorConfig);
        m_Status = "Reset all project settings to built-in defaults";
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Page")) {
        ResetCurrentSection(context);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_Status.empty() ? "Saved instantly" : m_Status.c_str());

    ImGui::Separator();
    ImGui::BeginChild("##project_settings_nav", ImVec2(210.0f, 0.0f), true);
    DrawNavigation();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##project_settings_content", ImVec2(0.0f, 0.0f), false);

    switch (m_Section) {
    case Section::EditorApplication:
        DrawEditorApplication(context);
        break;
    case Section::EditorWindowInput:
        DrawEditorWindowInput(context);
        break;
    case Section::EditorGraphics:
        DrawEditorGraphics(context);
        break;
    case Section::EditorLightingShadows:
        DrawEditorLightingShadows(context);
        break;
    case Section::EditorSceneAutosave:
        DrawEditorSceneAutosave(context);
        break;
    case Section::EditorAssets:
        DrawEditorAssets(context);
        break;
    case Section::EditorAudio:
        DrawEditorAudio(context);
        break;
    case Section::EditorPhysicsPreview:
        DrawEditorPhysicsPreview(context);
        break;
    case Section::EditorGameplayPreview:
        DrawEditorGameplayPreview(context);
        break;
    case Section::ServerApplication:
        DrawServerApplication(context);
        break;
    case Section::ServerSimulation:
        DrawServerSimulation(context);
        break;
    case Section::ServerPhysics:
        DrawServerPhysics(context);
        break;
    case Section::ServerGameplay:
        DrawServerGameplay(context);
        break;
    case Section::ServerStartupScene:
        DrawServerStartupScene(context);
        break;
    case Section::Preferences:
        DrawPreferences(context);
        break;
    }

    ImGui::EndChild();
    EndWindow();
}

void ProjectSettingsPanel::EnsureLoaded(EditorContext& context) {
    const std::filesystem::path editorPath =
        context.configPath.empty() ? Paths::ConfigFile("editor.toml") : context.configPath;
    if (m_Loaded && m_EditorPath == editorPath) {
        return;
    }
    m_EditorPath = editorPath;
    m_UserPreferencesPath = EditorSettings::DefaultPath();
    LoadAll(context);
}

void ProjectSettingsPanel::LoadAll(EditorContext& context) {
    auto loadOverlay = [&](Config& config, const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            return;
        }
        Config overlay;
        if (const Status status = overlay.Load(path); !status) {
            m_Status = status.error;
            return;
        }
        config.MergeFrom(overlay);
    };
    m_Status.clear();
    m_DefaultConfig = MakeDefaultRuntimeConfig();
    m_EditorConfig = m_DefaultConfig;
    loadOverlay(m_EditorConfig, m_EditorPath);
    m_ServerConfig = m_EditorConfig;
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    RefreshDerivedSettings();
    ApplyEditorAudioCueMap(context, m_EditorConfig);
    m_Loaded = true;
}

void ProjectSettingsPanel::RefreshDerivedSettings() {
    m_DefaultRenderer = MakeDefaultRendererSettings();
    LoadRendererSettings(m_DefaultConfig, m_DefaultRenderer);
    m_EditorRenderer = m_DefaultRenderer;
    LoadRendererSettings(m_EditorConfig, m_EditorRenderer);

    m_DefaultPhysics = MakeDefaultPhysicsSettings();
    LoadPhysicsSettings(m_DefaultConfig, m_DefaultPhysics);
    m_EditorPhysics = m_DefaultPhysics;
    LoadPhysicsSettings(m_EditorConfig, m_EditorPhysics);
    m_ServerPhysics = m_DefaultPhysics;
    LoadPhysicsSettings(m_ServerConfig, m_ServerPhysics);

    m_DefaultGameplay = LoadGameplaySettings(m_DefaultConfig);
    m_EditorGameplay = LoadGameplaySettings(m_EditorConfig);
    m_ServerGameplay = LoadGameplaySettings(m_ServerConfig);
}

void ProjectSettingsPanel::ResetCurrentSection(EditorContext& context) {
    switch (m_Section) {
    case Section::EditorApplication:
        ResetConfigString(m_EditorConfig, m_DefaultConfig, "app.name", "Hockey Map Editor");
        ResetConfigInt(m_EditorConfig, m_DefaultConfig, "app.target_fps", 0);
        m_EditorRestartRequired = true;
        SaveEditorConfig(context);
        break;
    case Section::EditorWindowInput:
        ResetConfigString(m_EditorConfig, m_DefaultConfig, "window.title", "Hockey Map Editor");
        ResetConfigInt(m_EditorConfig, m_DefaultConfig, "window.width", 1600);
        ResetConfigInt(m_EditorConfig, m_DefaultConfig, "window.height", 900);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "window.resizable", true);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "window.maximized", true);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "window.start_centered", true);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "input.gamepad_enabled", true);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "input.mouse_capture", false);
        Input::SetGamepadEnabled(m_EditorConfig.GetBool("input.gamepad_enabled", true));
        m_EditorRestartRequired = true;
        SaveEditorConfig(context);
        break;
    case Section::EditorGraphics:
    case Section::EditorLightingShadows:
        m_EditorRenderer = m_DefaultRenderer;
        SaveRendererSettings(m_EditorConfig, m_EditorRenderer);
        SaveEditorConfig(context);
        if (context.renderer != nullptr && context.renderer->IsInitialized()) {
            context.renderer->ApplySettings(m_EditorRenderer);
        }
        break;
    case Section::EditorSceneAutosave:
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "scene.autosave_enabled", true);
        ResetConfigInt(m_EditorConfig, m_DefaultConfig, "scene.autosave_interval_seconds", 120);
        context.settings.autosaveEnabled = m_EditorConfig.GetBool("scene.autosave_enabled", true);
        context.settings.autosaveIntervalSeconds = m_EditorConfig.GetInt("scene.autosave_interval_seconds", 120);
        SaveEditorConfig(context);
        break;
    case Section::EditorAssets:
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_discover", false);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_import", false);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_cook_dirty", false);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "assets.hot_reload", false);
        ResetConfigString(m_EditorConfig, m_DefaultConfig, "assets.raw_path", "");
        ResetConfigString(m_EditorConfig, m_DefaultConfig, "assets.cooked_path", "");
        context.settings.assetsAutoImport = m_EditorConfig.GetBool("assets.auto_import", false);
        context.settings.assetsAutoCookDirty = m_EditorConfig.GetBool("assets.auto_cook_dirty", false);
        context.settings.assetsHotReload = m_EditorConfig.GetBool("assets.hot_reload", false);
        if (context.assetManager != nullptr) {
            if (context.settings.assetsHotReload) {
                context.assetManager->StartWatching();
            } else {
                context.assetManager->StopWatching();
            }
        }
        m_EditorRestartRequired = true;
        SaveEditorConfig(context);
        break;
    case Section::EditorAudio:
        SaveDefaultHockeyAudioCuePaths(m_EditorConfig);
        SaveEditorConfig(context);
        ApplyEditorAudioCueMap(context, m_EditorConfig);
        break;
    case Section::EditorPhysicsPreview:
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "physics.enabled", true);
        m_EditorPhysics = m_DefaultPhysics;
        SavePhysicsSettings(m_EditorConfig, m_EditorPhysics);
        SaveEditorConfig(context);
        if (context.physicsPreview != nullptr) {
            context.physicsPreview->Configure(m_EditorPhysics);
        }
        break;
    case Section::EditorGameplayPreview:
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "gameplay.preview_enabled", false);
        ResetConfigBool(m_EditorConfig, m_DefaultConfig, "gameplay.validate_on_load", context.settings.validateAfterLoad);
        m_EditorGameplay = m_DefaultGameplay;
        SaveGameplaySettings(m_EditorConfig, m_EditorGameplay);
        context.settings.validateAfterLoad =
            m_EditorConfig.GetBool("gameplay.validate_on_load", context.settings.validateAfterLoad);
        SaveEditorConfig(context);
        if (context.gameplayPreview != nullptr) {
            context.gameplayPreview->Configure(m_EditorGameplay);
        }
        break;
    case Section::ServerApplication:
        ResetConfigBool(m_ServerConfig, m_DefaultConfig, "app.sleep_when_idle", true);
        ResetConfigString(m_ServerConfig, m_DefaultConfig, "server.name", "Local Hockey Server");
        ResetConfigInt(m_ServerConfig, m_DefaultConfig, "server.max_players", 8);
        ResetConfigInt(m_ServerConfig, m_DefaultConfig, "server.port", 27020);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
        break;
    case Section::ServerSimulation:
        ResetConfigFloat(m_ServerConfig, m_DefaultConfig, "server.tick_rate", 120.0f);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
        break;
    case Section::ServerPhysics:
        ResetConfigBool(m_ServerConfig, m_DefaultConfig, "physics.enabled", true);
        m_ServerPhysics = m_DefaultPhysics;
        SavePhysicsSettings(m_ServerConfig, m_ServerPhysics);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
        break;
    case Section::ServerGameplay:
        ResetConfigBool(m_ServerConfig, m_DefaultConfig, "gameplay.authoritative", true);
        m_ServerGameplay = m_DefaultGameplay;
        SaveGameplaySettings(m_ServerConfig, m_ServerGameplay);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
        break;
    case Section::ServerStartupScene:
        ResetConfigString(m_ServerConfig, m_DefaultConfig, "scene.startup_scene", "");
        ResetConfigBool(m_ServerConfig, m_DefaultConfig, "scene.validate_on_load", true);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
        break;
    case Section::Preferences:
        context.settings = EditorSettings{};
        if (context.assetManager != nullptr) {
            if (context.settings.assetsHotReload) {
                context.assetManager->StartWatching();
            } else {
                context.assetManager->StopWatching();
            }
        }
        SaveUserPreferences(context);
        m_Status = "Reset Preferences to built-in defaults";
        return;
    }

    m_Status = "Reset Page to built-in defaults";
}

void ProjectSettingsPanel::DrawNavigation() {
    auto item = [&](const char* label, Section section) {
        if (ImGui::Selectable(label, m_Section == section)) {
            m_Section = section;
        }
    };

    ImGui::TextUnformatted("Editor");
    ImGui::PushID("Editor");
    item("Application", Section::EditorApplication);
    item("Window / Input", Section::EditorWindowInput);
    item("Graphics", Section::EditorGraphics);
    item("Lighting & Shadows", Section::EditorLightingShadows);
    item("Scene / Autosave", Section::EditorSceneAutosave);
    item("Assets", Section::EditorAssets);
    item("Audio", Section::EditorAudio);
    item("Physics Preview", Section::EditorPhysicsPreview);
    item("Gameplay Preview", Section::EditorGameplayPreview);
    ImGui::PopID();
    ImGui::Separator();
    ImGui::TextUnformatted("Server Build Defaults");
    ImGui::PushID("Server");
    item("Application Defaults", Section::ServerApplication);
    item("Simulation Defaults", Section::ServerSimulation);
    item("Physics", Section::ServerPhysics);
    item("Gameplay", Section::ServerGameplay);
    item("Startup Scene", Section::ServerStartupScene);
    ImGui::PopID();
    ImGui::Separator();
    item("Preferences", Section::Preferences);
}

void ProjectSettingsPanel::DrawEditorApplication(EditorContext& context) {
    ImGui::TextUnformatted("Editor Application");
    ImGui::Separator();
    bool restart = false;
    restart |= DrawConfigString(m_EditorConfig, m_DefaultConfig, "app.name", "Application name", "Hockey Map Editor",
                                "Name used by the editor process while authoring and previewing scenes. This affects "
                                "window/app identity but not packaged game branding.");
    restart |= DrawConfigInt(m_EditorConfig, m_DefaultConfig, "app.target_fps", "Target FPS", 0, 0, 1000,
                             "Caps editor update cadence while inspecting scenes. Use 144 for a responsive high-refresh "
                             "workspace; 0 leaves the editor uncapped for profiling.");
    if (restart) {
        m_EditorRestartRequired = true;
        SaveEditorConfig(context);
    }
    DrawRestartBadge(m_EditorRestartRequired, "Restart required");
}

void ProjectSettingsPanel::DrawEditorWindowInput(EditorContext& context) {
    ImGui::TextUnformatted("Editor Window / Input");
    DrawRestartBadge(m_EditorRestartRequired, "Restart required");
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= DrawConfigString(m_EditorConfig, m_DefaultConfig, "window.title", "Title", "Hockey Map Editor",
                                    "Title shown on the editor window used to view and edit scenes.");
        changed |= DrawConfigInt(m_EditorConfig, m_DefaultConfig, "window.width", "Width", 1600, 320, 7680,
                                 "Initial editor window width. Larger values show more of the scene and panels at "
                                 "startup; smaller values are useful when checking compact layouts.");
        changed |= DrawConfigInt(m_EditorConfig, m_DefaultConfig, "window.height", "Height", 900, 240, 4320,
                                 "Initial editor window height. Larger values show more scene viewport area at "
                                 "startup.");
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "window.resizable", "Resizable", true,
                                  "Allows resizing the editor window while inspecting the scene layout.");
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "window.maximized", "Maximized", true,
                                  "Starts the editor maximized so the scene and tool panels get the largest workspace. "
                                  "Disable this only when testing fixed-size window startup behavior.");
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "window.start_centered", "Start centered", true,
                                  "Centers the editor window before loading the scene workspace.");
        if (changed) {
            m_EditorRestartRequired = true;
            SaveEditorConfig(context);
        }
    }

    if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        const bool gamepadDefault = m_DefaultConfig.GetBool("input.gamepad_enabled", true);
        bool gamepad = m_EditorConfig.GetBool("input.gamepad_enabled", gamepadDefault);
        const bool gamepadEdited = ImGui::Checkbox("Gamepad enabled", &gamepad);
        ShowSettingTooltip("Gamepad enabled", SettingValueKind::Boolean,
                           "Allows gamepad input while previewing or testing scene interactions in the editor. "
                           "Keep this off if controller input should be reserved for the running game client; turn it on "
                           "when testing couch-play or goalie/skater controls directly in editor preview.",
                           FormatBool(gamepadDefault));
        const bool gamepadReset = DrawResetSettingButton("Gamepad enabled");
        if (gamepadReset) {
            gamepad = gamepadDefault;
        }
        if (gamepadEdited || gamepadReset) {
            m_EditorConfig.SetBool("input.gamepad_enabled", gamepad);
            Input::SetGamepadEnabled(gamepad);
            SaveEditorConfig(context);
        }
        if (DrawConfigBool(m_EditorConfig, m_DefaultConfig, "input.mouse_capture", "Mouse capture", false,
                           "Captures the mouse for editor camera or preview controls when scene interaction needs "
                           "relative input. Leave this off by default so UI panels remain easy to use; enable it "
                           "when testing camera/gameplay controls that need continuous mouse deltas.")) {
            m_EditorRestartRequired = true;
            SaveEditorConfig(context);
        }
    }
}

void ProjectSettingsPanel::DrawEditorSceneAutosave(EditorContext& context) {
    ImGui::TextUnformatted("Editor Scene / Autosave");
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "scene.autosave_enabled", "Autosave enabled",
                                  context.settings.autosaveEnabled,
                                  "Writes backup scene files automatically so authoring changes are recoverable. Keep "
                                  "this on for normal editor work; turn it off only for controlled save/load tests.");
        changed |= DrawConfigInt(m_EditorConfig, m_DefaultConfig, "scene.autosave_interval_seconds", "Autosave interval seconds",
                                 context.settings.autosaveIntervalSeconds, 5, 3600,
                                 "Time between automatic scene backups while editing. Shorter intervals protect more "
                                 "work but perform more file writes; longer intervals reduce interruptions.");
        if (changed) {
            context.settings.autosaveEnabled =
                m_EditorConfig.GetBool("scene.autosave_enabled", context.settings.autosaveEnabled);
            context.settings.autosaveIntervalSeconds =
                m_EditorConfig.GetInt("scene.autosave_interval_seconds", context.settings.autosaveIntervalSeconds);
            SaveEditorConfig(context);
        }
    }
}

void ProjectSettingsPanel::DrawEditorAssets(EditorContext& context) {
    ImGui::TextUnformatted("Editor Assets");
    DrawRestartBadge(m_EditorRestartRequired, "Restart required");
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_discover", "Auto discover", false,
                                  "Scans asset folders for files that can be used by scenes. Enable it for active asset "
                                  "authoring; disable it when you need deterministic startup without discovery work.");
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_import", "Auto import", false,
                                  "Imports newly discovered source assets so scenes can reference them without manual "
                                  "steps. This speeds iteration but can create metadata churn when dropping many files.");
        changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "assets.auto_cook_dirty", "Auto cook dirty", false,
                                  "Rebuilds changed cooked assets so scene previews use current meshes, materials, "
                                  "textures, and audio. Keep it on for live content iteration; disable it when testing "
                                  "cook failures or package inputs.");
        const bool hotReloadDefault = m_DefaultConfig.GetBool("assets.hot_reload", false);
        bool hotReload = m_EditorConfig.GetBool("assets.hot_reload", hotReloadDefault);
        const bool hotReloadEdited = ImGui::Checkbox("Hot reload", &hotReload);
        ShowSettingTooltip("Hot reload", SettingValueKind::Boolean,
                           "Hot reload updates loaded scene assets when files change on disk. Leave it on for fast "
                           "material, mesh, audio, and texture iteration; turn it off when debugging import/cook order "
                           "or when file watcher churn is distracting.",
                           FormatBool(hotReloadDefault));
        const bool hotReloadReset = DrawResetSettingButton("Hot reload");
        if (hotReloadReset) {
            hotReload = hotReloadDefault;
        }
        if (hotReloadEdited || hotReloadReset) {
            m_EditorConfig.SetBool("assets.hot_reload", hotReload);
            context.settings.assetsHotReload = hotReload;
            if (context.assetManager != nullptr) {
                if (hotReload) {
                    context.assetManager->StartWatching();
                } else {
                    context.assetManager->StopWatching();
                }
            }
            changed = true;
        }
        bool pathChanged = false;
        pathChanged |= DrawConfigString(m_EditorConfig, m_DefaultConfig, "assets.raw_path", "Raw path", "",
                                        "Source asset folder used for scene-importable files. Empty uses the project "
                                        "default data/raw path; override only for temporary external content roots.");
        pathChanged |= DrawConfigString(m_EditorConfig, m_DefaultConfig, "assets.cooked_path", "Cooked path", "",
                                        "Cooked asset folder loaded by scene previews and runtime builds. Empty uses "
                                        "the project default data/cooked path; override when validating packaged data.");
        if (pathChanged) {
            m_EditorRestartRequired = true;
        }
        if (changed || pathChanged) {
            context.settings.assetsAutoImport = m_EditorConfig.GetBool("assets.auto_import", false);
            context.settings.assetsAutoCookDirty = m_EditorConfig.GetBool("assets.auto_cook_dirty", false);
            SaveEditorConfig(context);
        }
    }
}

void ProjectSettingsPanel::DrawEditorAudio(EditorContext& context) {
    ImGui::TextUnformatted("Editor Audio");
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Audio Cues", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        for (const AudioCueConfigBinding& binding : HockeyAudioCueConfigBindings()) {
            changed |= DrawAudioCueRow(m_EditorConfig, binding, context.assetManager, context.audioPreview);
        }
        if (changed) {
            SaveEditorConfig(context);
            ApplyEditorAudioCueMap(context, m_EditorConfig);
        }
    }
}

void ProjectSettingsPanel::DrawEditorGraphics(EditorContext& context) {
    ImGui::TextUnformatted("Editor Graphics");
    ImGui::Separator();
    if (DrawRendererSettings(m_EditorRenderer, m_DefaultRenderer)) {
        SaveRendererSettings(m_EditorConfig, m_EditorRenderer);
        SaveEditorConfig(context);
        if (context.renderer != nullptr && context.renderer->IsInitialized()) {
            context.renderer->ApplySettings(m_EditorRenderer);
        }
    }
}

void ProjectSettingsPanel::DrawEditorLightingShadows(EditorContext& context) {
    ImGui::TextUnformatted("Editor Lighting & Shadows");
    ImGui::Separator();
    if (DrawLightingShadowSettings(m_EditorRenderer, m_DefaultRenderer)) {
        SaveRendererSettings(m_EditorConfig, m_EditorRenderer);
        SaveEditorConfig(context);
        if (context.renderer != nullptr && context.renderer->IsInitialized()) {
            context.renderer->ApplySettings(m_EditorRenderer);
        }
    }
}

void ProjectSettingsPanel::DrawEditorPhysicsPreview(EditorContext& context) {
    ImGui::TextUnformatted("Editor Physics Preview");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "physics.enabled", "Physics enabled", true,
                              "Enables physics simulation for editor scene preview. Turn it off for static art/layout "
                              "inspection; keep it on to validate puck, player, board, and trigger behavior.");
    changed |= DrawPhysicsSettings(m_EditorPhysics, m_DefaultPhysics, false);
    if (changed) {
        SavePhysicsSettings(m_EditorConfig, m_EditorPhysics);
        SaveEditorConfig(context);
        if (context.physicsPreview != nullptr) {
            context.physicsPreview->Configure(m_EditorPhysics);
        }
    }
}

void ProjectSettingsPanel::DrawEditorGameplayPreview(EditorContext& context) {
    ImGui::TextUnformatted("Editor Gameplay Preview");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "gameplay.preview_enabled", "Preview enabled", false,
                              "Enables gameplay preview so the authored scene can run hockey rules in the editor. "
                              "Disable it for pure scene editing; enable it when checking spawns, faceoffs, scoring, "
                              "possession, and local input.");
    changed |= DrawConfigBool(m_EditorConfig, m_DefaultConfig, "gameplay.validate_on_load", "Validate on load",
                              context.settings.validateAfterLoad,
                              "Validate on load checks the scene for missing required gameplay objects before play. "
                              "Keep it on so missing puck, rink, goal, and spawn markers are reported immediately.");
    changed |= DrawGameplaySettings(m_EditorGameplay, m_DefaultGameplay);
    if (changed) {
        SaveGameplaySettings(m_EditorConfig, m_EditorGameplay);
        context.settings.validateAfterLoad =
            m_EditorConfig.GetBool("gameplay.validate_on_load", context.settings.validateAfterLoad);
        SaveEditorConfig(context);
        if (context.gameplayPreview != nullptr) {
            context.gameplayPreview->Configure(m_EditorGameplay);
        }
    }
}

void ProjectSettingsPanel::DrawServerApplication(EditorContext& context) {
    ImGui::TextUnformatted("Server Build Defaults");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, m_DefaultConfig, "app.sleep_when_idle", "Sleep when idle", true,
                              "Lets the headless server sleep when no scene simulation work is active. Keep this on "
                              "for local/dev servers to reduce idle CPU use; disable only for profiling wake latency.");
    changed |= DrawConfigString(m_ServerConfig, m_DefaultConfig, "server.name", "Server name", "Local Hockey Server",
                                "Name advertised for the server that owns the authoritative scene. This is the default "
                                "label future clients and lobby tools should show for local packages.");
    changed |= DrawConfigInt(m_ServerConfig, m_DefaultConfig, "server.max_players", "Max players", 8, 1, 128,
                             "Maximum connected players allowed to join the authoritative scene. The 4v4 design uses "
                             "8 players; raising this is only useful for future spectators or larger test modes.");
    changed |= DrawConfigInt(m_ServerConfig, m_DefaultConfig, "server.port", "Port", 27020, 1, 65535,
                             "Network port clients use to connect to the authoritative scene server. Change it when "
                             "running multiple local servers or avoiding a port conflict.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
    }
}

void ProjectSettingsPanel::DrawServerSimulation(EditorContext& context) {
    ImGui::TextUnformatted("Server Simulation Defaults");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigFloat(m_ServerConfig, m_DefaultConfig, "server.tick_rate", "Tick rate", 120.0f, 1.0f, 240.0f,
                               "Authoritative scene simulation ticks per second. 120 Hz is recommended for responsive "
                               "4v4 hockey, accurate puck/stick timing, and future snapshot cadence. Lower values save "
                               "CPU and bandwidth but make contacts, steals, and shots feel less immediate.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
    }
}

void ProjectSettingsPanel::DrawServerStartupScene(EditorContext& context) {
    ImGui::TextUnformatted("Server Startup Scene");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ServerConfig, m_DefaultConfig, "scene.startup_scene", "Startup scene", "",
                                "Startup scene is the scene loaded when players launch the build and when the "
                                "headless server starts authoritative simulation. Use the main rink scene for normal "
                                "packages and a focused test scene for server validation.");
    changed |= DrawConfigBool(m_ServerConfig, m_DefaultConfig, "scene.validate_on_load", "Validate on load", true,
                              "Checks the startup scene for required hockey gameplay objects before the server runs. "
                              "Keep this on so broken packaged scenes fail early instead of simulating invalid matches.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
    }
}

void ProjectSettingsPanel::DrawServerPhysics(EditorContext& context) {
    ImGui::TextUnformatted("Server Physics");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, m_DefaultConfig, "physics.enabled", "Physics enabled", true,
                              "Enables authoritative physics simulation for the server scene. Keep this on for hockey "
                              "matches because puck, boards, triggers, and player bodies need server-owned physics.");
    changed |= DrawPhysicsSettings(m_ServerPhysics, m_DefaultPhysics, true);
    if (changed) {
        SavePhysicsSettings(m_ServerConfig, m_ServerPhysics);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
    }
}

void ProjectSettingsPanel::DrawServerGameplay(EditorContext& context) {
    ImGui::TextUnformatted("Server Gameplay");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, m_DefaultConfig, "gameplay.authoritative", "Authoritative", true,
                              "Makes the server own gameplay truth for the scene, including scoring, rules, and "
                              "snapshots. Keep this on for dedicated servers; disabling it is only useful for isolated "
                              "client/local-play experiments.");
    changed |= DrawGameplaySettings(m_ServerGameplay, m_DefaultGameplay);
    if (changed) {
        SaveGameplaySettings(m_ServerConfig, m_ServerGameplay);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults(context);
    }
}

void ProjectSettingsPanel::DrawPreferences(EditorContext& context) {
    ImGui::TextUnformatted("Editor Preferences");
    ImGui::Separator();
    if (ImGui::Button("Reset Preferences")) {
        ResetCurrentSection(context);
    }
    ImGui::Separator();
    EditorSettings& s = context.settings;
    const EditorSettings defaults;
    bool changed = false;

    if (ImGui::CollapsingHeader("Autosave", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Autosave enabled", s.autosaveEnabled, defaults.autosaveEnabled,
                                "Automatically saves backup copies of the open scene while editing. Keep it on during "
                                "normal authoring so layout, prefab, and gameplay-marker edits are recoverable.");
        ImGui::BeginDisabled(!s.autosaveEnabled);
        changed |= DrawInt("Interval seconds", s.autosaveIntervalSeconds, defaults.autosaveIntervalSeconds, 5, 3600,
                           "How often the editor writes scene autosave files. Shorter intervals reduce lost work after "
                           "a crash but write more often; longer intervals are quieter during heavy imports.");
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Validate before save", s.validateBeforeSave, defaults.validateBeforeSave,
                                "Checks the scene for missing hockey gameplay objects before writing it to disk. Leave "
                                "this on so broken rink, puck, goal, and spawn setups are caught before commit.");
        changed |= DrawCheckbox("Validate after load", s.validateAfterLoad, defaults.validateAfterLoad,
                                "Checks the scene after opening it so missing puck, rink, goal, and spawn markers are "
                                "reported immediately.");
    }

    if (ImGui::CollapsingHeader("Grid & Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Show grid", s.showGrid, defaults.showGrid,
                                "Shows the ground grid in the scene viewport for rink and object alignment. Turn it off "
                                "when reviewing final art or screenshots without construction guides.");
        changed |= DrawFloat("Grid spacing", s.gridSpacing, defaults.gridSpacing, 0.05f, 0.05f, 100.0f, "%.2f",
                             "Distance between scene viewport grid lines. Smaller spacing helps precise prop placement; "
                             "larger spacing makes full-rink layout easier to read.");
        changed |= DrawCheckbox("Snap enabled", s.snapEnabled, defaults.snapEnabled,
                                "Constrains transform edits to scene-friendly increments for cleaner placement. Enable "
                                "it for boards, goals, and markers that need exact alignment.");
        changed |= DrawFloat("Move snap", s.moveSnap, defaults.moveSnap, 0.01f, 0.001f, 100.0f, "%.3f",
                             "World-unit increment used when moving scene objects with snapping enabled. Smaller values "
                             "allow fine tuning; larger values keep rink layout edits aligned.");
        changed |= DrawFloat("Rotate snap degrees", s.rotateSnapDegrees, defaults.rotateSnapDegrees, 0.5f, 1.0f,
                             180.0f, "%.1f",
                             "Degree increment used when rotating scene objects with snapping enabled. Common values "
                             "like 15, 30, or 45 degrees keep boards, cameras, and lights aligned.");
        changed |= DrawFloat("Scale snap", s.scaleSnap, defaults.scaleSnap, 0.01f, 0.001f, 100.0f, "%.3f",
                             "Scale increment used when resizing scene objects with snapping enabled. Smaller values "
                             "help tune collider fit; larger values keep blocker/proxy edits coarse and predictable.");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawFloat("Move speed", s.cameraMoveSpeed, defaults.cameraMoveSpeed, 0.1f, 0.1f, 1000.0f, "%.2f",
                             "Base speed for flying the editor camera through the scene. Higher values cross the rink "
                             "quickly but make close prop and collider inspection harder.");
        changed |= DrawFloat("Fast multiplier", s.cameraFastMultiplier, defaults.cameraFastMultiplier, 0.1f, 1.0f,
                             50.0f, "%.2f",
                             "Multiplier applied to editor camera movement for crossing large scenes quickly. Higher "
                             "values are useful for arena-scale traversal but can overshoot small selections.");
        changed |= DrawFloat("Mouse sensitivity", s.cameraMouseSensitivity, defaults.cameraMouseSensitivity, 0.005f,
                             0.01f, 2.0f, "%.3f",
                             "Mouse-look sensitivity for orbiting or flying the scene camera. Lower values help precise "
                             "selection and framing; higher values turn faster in large arenas.");
    }

    if (ImGui::CollapsingHeader("Startup", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Restore last scene on launch", s.restoreLastScene, defaults.restoreLastScene,
                                "Reopens the last edited scene when the editor starts. Keep it on for daily authoring; "
                                "turn it off when testing startup scene selection from a clean editor state.");
    }

    if (ImGui::CollapsingHeader("Asset Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Auto import on change", s.assetsAutoImport, defaults.assetsAutoImport,
                                "Imports changed source assets so scene references can refresh without manual import. "
                                "Enable it during content iteration; disable it to avoid metadata churn.");
        changed |= DrawCheckbox("Auto cook dirty assets", s.assetsAutoCookDirty, defaults.assetsAutoCookDirty,
                                "Cooks modified assets so scene previews use current runtime-ready data. Enable it for "
                                "fast visual/audio iteration; disable it when diagnosing cooker output.");
        bool hotReload = s.assetsHotReload;
        const bool hotReloadEdited = ImGui::Checkbox("Hot reload", &hotReload);
        ShowSettingTooltip("Hot reload", SettingValueKind::Boolean,
                           "Reloads changed assets into open scenes when the asset watcher detects updates. Leave it off "
                           "for the most predictable editor state; turn it on while iterating materials, meshes, audio, "
                           "or textures so scene previews refresh automatically.",
                           FormatBool(defaults.assetsHotReload));
        const bool hotReloadReset = DrawResetSettingButton("Hot reload");
        if (hotReloadReset) {
            hotReload = defaults.assetsHotReload;
        }
        if (hotReloadEdited || hotReloadReset) {
            s.assetsHotReload = hotReload;
            changed = true;
            if (context.assetManager != nullptr) {
                if (s.assetsHotReload) {
                    context.assetManager->StartWatching();
                } else {
                    context.assetManager->StopWatching();
                }
            }
        }
    }

    if (changed) {
        SaveUserPreferences(context);
    }
}

void ProjectSettingsPanel::SaveEditorConfig(EditorContext& context) {
    if (const Status status = m_EditorConfig.Save(m_EditorPath); !status) {
        m_Status = status.error;
        return;
    }
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    m_ServerConfig = m_EditorConfig;
    m_ServerPhysics = m_EditorPhysics;
    m_ServerGameplay = m_EditorGameplay;
    m_Status = "Saved " + m_EditorPath.filename().string();
}

void ProjectSettingsPanel::SaveServerBuildDefaults(EditorContext& context) {
    m_EditorConfig = m_ServerConfig;
    if (const Status status = m_EditorConfig.Save(m_EditorPath); !status) {
        m_Status = status.error;
        return;
    }
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    m_EditorPhysics = m_ServerPhysics;
    m_EditorGameplay = m_ServerGameplay;
    m_Status = "Saved " + m_EditorPath.filename().string();
}

void ProjectSettingsPanel::SaveUserPreferences(EditorContext& context) {
    if (const Status status = context.settings.Save(m_UserPreferencesPath); !status) {
        m_Status = status.error;
        return;
    }
    m_Status = "Saved " + m_UserPreferencesPath.filename().string();
}

} // namespace Hockey
