#include "Hockey/Editor/Panels/ProjectSettingsPanel.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

template <typename T, std::size_t Count>
bool DrawEnumCombo(const char* label, T& value, const std::array<T, Count>& values) {
    bool changed = false;
    if (ImGui::BeginCombo(label, ToString(value))) {
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
    return changed;
}

bool DrawUInt(const char* label, std::uint32_t& value, int minValue, int maxValue) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    if (ImGui::DragInt(label, &temp, 1.0f, minValue, maxValue)) {
        value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawUIntPowerOfTwoOrZero(const char* label, std::uint32_t& value, int minValue, int maxValue) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    if (ImGui::DragInt(label, &temp, 64.0f, 0, maxValue)) {
        if (temp <= 0) {
            value = 0;
        } else {
            value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
        }
        return true;
    }
    return false;
}

bool DrawFloat(const char* label, float& value, float speed, float minValue, float maxValue, const char* format) {
    return ImGui::DragFloat(label, &value, speed, minValue, maxValue, format);
}

bool DrawConfigBool(Config& config, const char* key, const char* label, bool fallback) {
    bool value = config.GetBool(key, fallback);
    if (ImGui::Checkbox(label, &value)) {
        config.SetBool(key, value);
        return true;
    }
    return false;
}

bool DrawConfigInt(Config& config, const char* key, const char* label, int fallback, int minValue, int maxValue) {
    int value = config.GetInt(key, fallback);
    if (ImGui::DragInt(label, &value, 1.0f, minValue, maxValue)) {
        config.SetInt(key, std::clamp(value, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawConfigFloat(Config& config, const char* key, const char* label, float fallback, float minValue,
                     float maxValue) {
    float value = static_cast<float>(config.GetDouble(key, fallback));
    if (ImGui::DragFloat(label, &value, 0.05f, minValue, maxValue, "%.3f")) {
        config.SetDouble(key, value);
        return true;
    }
    return false;
}

bool DrawConfigString(Config& config, const char* key, const char* label, const char* fallback) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", config.GetString(key, fallback).c_str());
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        config.SetString(key, buffer);
        return true;
    }
    return false;
}

void DrawRestartBadge(bool restartRequired, const char* text) {
    if (restartRequired) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", text);
    }
}

bool DrawRendererSettings(RendererSettings& settings) {
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
        custom |= DrawEnumCombo("Display mode", settings.displayMode, kDisplayModes);
        custom |= DrawUInt("Resolution width", settings.resolutionWidth, 320, 7680);
        custom |= DrawUInt("Resolution height", settings.resolutionHeight, 240, 4320);
        custom |= DrawUInt("Refresh rate", settings.refreshRate, 0, 480);
        custom |= DrawUInt("Monitor index", settings.monitorIndex, 0, 16);
        custom |= ImGui::Checkbox("VSync", &settings.vsync);
        custom |= DrawUInt("FPS limit", settings.fpsLimit, 0, 1000);
        custom |= ImGui::Checkbox("HDR", &settings.hdr);
        custom |= DrawFloat("Brightness", settings.brightness, 0.01f, 0.1f, 4.0f, "%.2f");
        custom |= DrawFloat("Field of view", settings.fieldOfView, 0.1f, 1.0f, 179.0f, "%.1f");
    }

    if (ImGui::CollapsingHeader("Scaling", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawFloat("Render scale", settings.renderScale, 0.01f, 0.25f, 2.0f, "%.2f");
        custom |= ImGui::Checkbox("Dynamic resolution", &settings.dynamicResolution);
        custom |= DrawEnumCombo("Upscaler", settings.upscaler, kUpscalers);
        custom |= DrawFloat("Sharpening", settings.sharpening, 0.01f, 0.0f, 2.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        GraphicsPreset preset = settings.preset;
        if (DrawEnumCombo("Preset", preset, kPresets)) {
            settings = ApplyGraphicsPreset(preset, settings);
            changed = true;
        }
        custom |= DrawEnumCombo("Texture quality", settings.textureQuality, kTextureQualities);
        custom |= DrawUInt("Texture budget MB", settings.textureStreamingBudgetMB, 128, 32768);
        custom |= DrawUInt("Anisotropy", settings.anisotropy, 1, 16);
        custom |= DrawEnumCombo("Material quality", settings.materialQuality, kDetailQualities);
        custom |= DrawEnumCombo("Model quality", settings.modelQuality, kDetailQualities);
        custom |= DrawFloat("LOD distance multiplier", settings.lodDistanceMultiplier, 0.01f, 0.1f, 10.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= ImGui::Checkbox("Bloom", &settings.bloom);
        custom |= ImGui::Checkbox("Motion blur", &settings.motionBlur);
        custom |= ImGui::Checkbox("Depth of field", &settings.depthOfField);
        custom |= ImGui::Checkbox("Lens flare", &settings.lensFlare);
        custom |= DrawEnumCombo("Volumetric lighting", settings.volumetricLighting, kDetailQualities);
        custom |= DrawEnumCombo("Particle quality", settings.particleQuality, kDetailQualities);
        custom |= DrawEnumCombo("Anti-aliasing", settings.antiAliasing, kAntiAliasing);
        custom |= DrawEnumCombo("Tone mapper", settings.toneMapper, kToneMappers);
        custom |= ImGui::Checkbox("Film grain", &settings.filmGrain);
        custom |= ImGui::Checkbox("Chromatic aberration", &settings.chromaticAberration);
        custom |= ImGui::Checkbox("Vignette", &settings.vignette);
    }

    if (ImGui::CollapsingHeader("Hockey", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawEnumCombo("Ice quality", settings.iceQuality, kDetailQualities);
        custom |= DrawEnumCombo("Ice reflection", settings.iceReflectionQuality, kReflectionQualities);
        custom |= DrawEnumCombo("Ice scratches", settings.iceScratchQuality, kDetailQualities);
        custom |= DrawEnumCombo("Skate spray", settings.skateSprayQuality, kEffectQualities);
        custom |= DrawEnumCombo("Puck trail", settings.puckTrailQuality, kEffectQualities);
        custom |= DrawEnumCombo("Jersey quality", settings.jerseyQuality, kDetailQualities);
        custom |= DrawEnumCombo("Goal net quality", settings.goalNetQuality, kDetailQualities);
        custom |= DrawEnumCombo("Crowd quality", settings.crowdQuality, kDetailQualities);
        custom |= DrawEnumCombo("Arena detail", settings.arenaDetail, kDetailQualities);
        custom |= DrawEnumCombo("Board glass detail", settings.boardGlassDetail, kDetailQualities);
    }

    if (ImGui::CollapsingHeader("Debug Overlays", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= ImGui::Checkbox("Show FPS", &settings.showFPS);
        custom |= ImGui::Checkbox("Show frame time", &settings.showFrameTime);
        custom |= ImGui::Checkbox("Show GPU stats", &settings.showGPUStats);
        custom |= ImGui::Checkbox("Show network stats", &settings.showNetworkStats);
    }

    if (custom) {
        settings.preset = GraphicsPreset::Custom;
    }
    return changed || custom;
}

bool DrawLightingShadowSettings(RendererSettings& settings) {
    bool changed = false;

    if (ImGui::CollapsingHeader("Budgets", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawUInt("Max rendered lights", settings.maxRenderedLights, 0, 16);
        changed |= DrawUInt("Max local shadow tiles", settings.maxLocalShadowTiles, 0, 16);
    }

    if (ImGui::CollapsingHeader("Shadow Atlases", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawEnumCombo("Shadow quality", settings.shadowQuality,
                                 std::array{ShadowQuality::Off, ShadowQuality::Low, ShadowQuality::Medium,
                                            ShadowQuality::High, ShadowQuality::Ultra});
        changed |= DrawFloat("Shadow distance", settings.shadowDistance, 1.0f, 0.0f, 5000.0f, "%.0f");
        changed |=
            DrawUIntPowerOfTwoOrZero("Directional atlas", settings.directionalShadowAtlasResolution, 256, 16384);
        changed |= DrawUIntPowerOfTwoOrZero("Local atlas", settings.localShadowAtlasResolution, 256, 16384);
    }

    if (ImGui::CollapsingHeader("Directional Cascades", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawUInt("Cascade count", settings.shadowCascadeCount, 0, 4);
        changed |= DrawFloat("Split lambda", settings.shadowCascadeSplitLambda, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= DrawFloat("Overlap scale", settings.shadowCascadeOverlapScale, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= DrawFloat("Minimum overlap", settings.shadowCascadeMinOverlap, 0.25f, 0.0f, 100.0f, "%.2f");
        changed |= DrawFloat("Max overlap scale", settings.shadowCascadeMaxOverlapScale, 0.01f, 0.0f, 1.0f,
                             "%.2f");
        changed |= DrawFloat("Blend scale", settings.shadowCascadeBlendScale, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= DrawFloat("Minimum blend", settings.shadowCascadeMinBlend, 0.05f, 0.0f, 50.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Directional Filter & Bias", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawUInt("PCF radius", settings.directionalShadowPcfRadius, 0, 3);
        changed |= DrawFloat("Normal offset scale", settings.directionalShadowNormalOffsetScale, 0.01f, 0.0f,
                             4.0f, "%.2f");
        changed |= DrawFloat("Normal offset min", settings.directionalShadowNormalOffsetMin, 0.0005f, 0.0f,
                             0.25f, "%.4f");
        changed |= DrawFloat("Normal offset max", settings.directionalShadowNormalOffsetMax, 0.001f, 0.0f, 1.0f,
                             "%.4f");
        changed |= DrawFloat("Bias base", settings.directionalShadowBiasBase, 0.00005f, 0.0f, 0.02f, "%.5f");
        changed |= DrawFloat("Bias slope", settings.directionalShadowBiasSlope, 0.00005f, 0.0f, 0.05f, "%.5f");
        changed |= DrawFloat("Bias min", settings.directionalShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f");
        changed |= DrawFloat("Bias max", settings.directionalShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f");
        changed |= DrawFloat("Depth bias constant", settings.directionalShadowDepthBiasConstant, 0.05f, 0.0f,
                             8.0f, "%.2f");
        changed |= DrawFloat("Depth bias slope", settings.directionalShadowDepthBiasSlope, 0.05f, 0.0f, 8.0f,
                             "%.2f");
    }

    if (ImGui::CollapsingHeader("Contact Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Contact shadows", &settings.contactShadows);
        changed |= DrawFloat("Distance", settings.contactShadowDistance, 0.5f, 0.0f, 500.0f, "%.1f");
        changed |= DrawFloat("Strength", settings.contactShadowStrength, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= DrawFloat("Normal offset scale", settings.contactShadowNormalOffsetScale, 0.01f, 0.0f, 2.0f,
                             "%.2f");
        changed |= DrawFloat("Normal offset min", settings.contactShadowNormalOffsetMin, 0.0005f, 0.0f, 0.25f,
                             "%.4f");
        changed |= DrawFloat("Bias base", settings.contactShadowBiasBase, 0.00005f, 0.0f, 0.02f, "%.5f");
        changed |= DrawFloat("Bias slope", settings.contactShadowBiasSlope, 0.00005f, 0.0f, 0.05f, "%.5f");
        changed |= DrawFloat("Bias min", settings.contactShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f");
        changed |= DrawFloat("Bias max", settings.contactShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f");
    }

    if (ImGui::CollapsingHeader("Local Light Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawUInt("PCF radius", settings.localShadowPcfRadius, 0, 3);
        changed |= DrawFloat("Bias scale", settings.localShadowBiasScale, 0.00005f, 0.0f, 0.05f, "%.5f");
        changed |= DrawFloat("Bias min", settings.localShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f");
        changed |= DrawFloat("Bias max", settings.localShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f");
    }

    if (ImGui::CollapsingHeader("Ambient & Reflections", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawEnumCombo("Ambient occlusion", settings.aoQuality,
                                 std::array{AmbientOcclusionQuality::Off, AmbientOcclusionQuality::Low,
                                            AmbientOcclusionQuality::Medium, AmbientOcclusionQuality::High,
                                            AmbientOcclusionQuality::Ultra});
        changed |= DrawEnumCombo("Reflection quality", settings.reflectionQuality,
                                 std::array{ReflectionQuality::Off, ReflectionQuality::Low,
                                            ReflectionQuality::Medium, ReflectionQuality::High,
                                            ReflectionQuality::Ultra});
        changed |= DrawEnumCombo("Global illumination", settings.globalIlluminationQuality,
                                 std::array{DetailQuality::Low, DetailQuality::Medium, DetailQuality::High,
                                            DetailQuality::Ultra});
    }

    if (changed) {
        settings = ClampRendererSettings(settings);
        settings.preset = GraphicsPreset::Custom;
    }
    return changed;
}

bool DrawPhysicsSettings(PhysicsSettings& settings, bool serverMode) {
    bool changed = false;
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, 0.0005f, 0.001f, 0.1f, "%.4f");
    changed |= DrawUInt("Max bodies", settings.maxBodies, 1, 1000000);
    changed |= DrawUInt("Max body pairs", settings.maxBodyPairs, 1, 1000000);
    changed |= DrawUInt("Max contact constraints", settings.maxContactConstraints, 1, 1000000);
    changed |= DrawUInt("Temp allocator MB", settings.tempAllocatorSizeMB, 1, 4096);
    changed |= DrawUInt("Job thread count", settings.jobThreadCount, 0, 128);
    changed |= DrawFloat("Gravity X", settings.gravity.x, 0.01f, -100.0f, 100.0f, "%.2f");
    changed |= DrawFloat("Gravity Y", settings.gravity.y, 0.01f, -100.0f, 100.0f, "%.2f");
    changed |= DrawFloat("Gravity Z", settings.gravity.z, 0.01f, -100.0f, 100.0f, "%.2f");
    changed |= DrawUInt("Collision steps", settings.collisionSteps, 1, 32);
    changed |= DrawUInt("Integration substeps", settings.integrationSubsteps, 1, 32);
    if (serverMode) {
        changed |= ImGui::Checkbox("Deterministic mode", &settings.deterministicMode);
    }
    changed |= ImGui::Checkbox("Enable sleeping", &settings.enableSleeping);
    changed |= ImGui::Checkbox("Enable debug draw", &settings.enableDebugDraw);
    changed |= DrawFloat("World min Y", settings.worldMinY, 1.0f, -100000.0f, 0.0f, "%.0f");
    return changed;
}

bool DrawGameplaySettings(GameplaySettings& settings) {
    bool changed = false;
    changed |= ImGui::Checkbox("Enabled", &settings.enabled);
    changed |= DrawUInt("Target player count", settings.targetPlayerCount, 1, 64);
    changed |= DrawUInt("Skaters per team", settings.skatersPerTeam, 0, 16);
    changed |= DrawUInt("Goalies per team", settings.goaliesPerTeam, 0, 4);
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, 0.0005f, 0.001f, 0.1f, "%.4f");
    changed |= DrawFloat("Period length seconds", settings.periodLengthSeconds, 1.0f, 1.0f, 7200.0f, "%.0f");
    changed |= DrawUInt("Period count", settings.periodCount, 1, 12);
    changed |= ImGui::Checkbox("Stop clock after goal", &settings.stopClockAfterGoal);
    changed |= ImGui::Checkbox("Auto faceoff after goal", &settings.autoFaceoffAfterGoal);
    changed |= DrawFloat("Post-goal delay seconds", settings.postGoalDelaySeconds, 0.1f, 0.0f, 60.0f, "%.1f");
    changed |= ImGui::Checkbox("Allow body checking", &settings.allowBodyChecking);
    changed |= ImGui::Checkbox("Allow manual goalie", &settings.allowManualGoalie);
    changed |= ImGui::Checkbox("Allow out of play", &settings.allowOutOfPlay);
    changed |= ImGui::Checkbox("Debug draw gameplay", &settings.debugDrawGameplay);
    changed |= ImGui::Checkbox("Log gameplay events", &settings.logGameplayEvents);
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
    case Section::EditorPhysicsPreview:
        DrawEditorPhysicsPreview(context);
        break;
    case Section::EditorGameplayPreview:
        DrawEditorGameplayPreview(context);
        break;
    case Section::ClientApplication:
        DrawClientApplication();
        break;
    case Section::ClientWindowInput:
        DrawClientWindowInput();
        break;
    case Section::ClientGraphics:
        DrawClientGraphics();
        break;
    case Section::ClientLightingShadows:
        DrawClientLightingShadows();
        break;
    case Section::ClientPhysics:
        DrawClientPhysics();
        break;
    case Section::ClientGameplay:
        DrawClientGameplay();
        break;
    case Section::ClientStartupScene:
        DrawClientStartupScene();
        break;
    case Section::ServerApplication:
        DrawServerApplication();
        break;
    case Section::ServerSimulation:
        DrawServerSimulation();
        break;
    case Section::ServerPhysics:
        DrawServerPhysics();
        break;
    case Section::ServerGameplay:
        DrawServerGameplay();
        break;
    case Section::ServerStartupScene:
        DrawServerStartupScene();
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
    m_ClientPath = Paths::ConfigFile("client.toml");
    m_ServerPath = Paths::ConfigFile("server.toml");
    m_UserPreferencesPath = EditorSettings::DefaultPath();
    LoadAll(context);
}

void ProjectSettingsPanel::LoadAll(EditorContext& context) {
    auto load = [&](Config& config, const std::filesystem::path& path) {
        config = Config{};
        if (!std::filesystem::exists(path)) {
            return;
        }
        if (const Status status = config.Load(path); !status) {
            m_Status = status.error;
        }
    };
    m_Status.clear();
    load(m_EditorConfig, m_EditorPath);
    load(m_ClientConfig, m_ClientPath);
    load(m_ServerConfig, m_ServerPath);
    if (context.config != nullptr) {
        *context.config = m_EditorConfig;
    }
    RefreshDerivedSettings();
    m_Loaded = true;
}

void ProjectSettingsPanel::RefreshDerivedSettings() {
    m_EditorRenderer = MakeDefaultRendererSettings();
    LoadRendererSettings(m_EditorConfig, m_EditorRenderer);
    m_ClientRenderer = MakeDefaultRendererSettings();
    LoadRendererSettings(m_ClientConfig, m_ClientRenderer);

    m_EditorPhysics = MakeDefaultPhysicsSettings();
    LoadPhysicsSettings(m_EditorConfig, m_EditorPhysics);
    m_ClientPhysics = MakeDefaultPhysicsSettings();
    LoadPhysicsSettings(m_ClientConfig, m_ClientPhysics);
    m_ServerPhysics = MakeDefaultPhysicsSettings();
    LoadPhysicsSettings(m_ServerConfig, m_ServerPhysics);

    m_EditorGameplay = LoadGameplaySettings(m_EditorConfig);
    m_ClientGameplay = LoadGameplaySettings(m_ClientConfig);
    m_ServerGameplay = LoadGameplaySettings(m_ServerConfig);
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
    item("Physics Preview", Section::EditorPhysicsPreview);
    item("Gameplay Preview", Section::EditorGameplayPreview);
    ImGui::PopID();
    ImGui::Separator();
    ImGui::TextUnformatted("Client");
    ImGui::PushID("Client");
    item("Application", Section::ClientApplication);
    item("Window / Input", Section::ClientWindowInput);
    item("Graphics", Section::ClientGraphics);
    item("Lighting & Shadows", Section::ClientLightingShadows);
    item("Physics", Section::ClientPhysics);
    item("Gameplay", Section::ClientGameplay);
    item("Startup Scene", Section::ClientStartupScene);
    ImGui::PopID();
    ImGui::Separator();
    ImGui::TextUnformatted("Server");
    ImGui::PushID("Server");
    item("Application", Section::ServerApplication);
    item("Simulation", Section::ServerSimulation);
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
    restart |= DrawConfigString(m_EditorConfig, "app.name", "Application name", "Hockey Map Editor");
    restart |= DrawConfigInt(m_EditorConfig, "app.target_fps", "Target FPS", 0, 0, 1000);
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
        changed |= DrawConfigString(m_EditorConfig, "window.title", "Title", "Hockey Map Editor");
        changed |= DrawConfigInt(m_EditorConfig, "window.width", "Width", 1600, 320, 7680);
        changed |= DrawConfigInt(m_EditorConfig, "window.height", "Height", 900, 240, 4320);
        changed |= DrawConfigBool(m_EditorConfig, "window.resizable", "Resizable", true);
        changed |= DrawConfigBool(m_EditorConfig, "window.maximized", "Maximized", true);
        changed |= DrawConfigBool(m_EditorConfig, "window.start_centered", "Start centered", true);
        if (changed) {
            m_EditorRestartRequired = true;
            SaveEditorConfig(context);
        }
    }

    if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool gamepad = m_EditorConfig.GetBool("input.gamepad_enabled", true);
        if (ImGui::Checkbox("Gamepad enabled", &gamepad)) {
            m_EditorConfig.SetBool("input.gamepad_enabled", gamepad);
            Input::SetGamepadEnabled(gamepad);
            SaveEditorConfig(context);
        }
        if (DrawConfigBool(m_EditorConfig, "input.mouse_capture", "Mouse capture", false)) {
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
        changed |= DrawConfigBool(m_EditorConfig, "scene.autosave_enabled", "Autosave enabled",
                                  context.settings.autosaveEnabled);
        changed |= DrawConfigInt(m_EditorConfig, "scene.autosave_interval_seconds", "Autosave interval seconds",
                                 context.settings.autosaveIntervalSeconds, 5, 3600);
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
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_discover", "Auto discover", false);
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_import", "Auto import", false);
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_cook_dirty", "Auto cook dirty", false);
        bool hotReload = m_EditorConfig.GetBool("assets.hot_reload", false);
        if (ImGui::Checkbox("Hot reload", &hotReload)) {
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
        pathChanged |= DrawConfigString(m_EditorConfig, "assets.raw_path", "Raw path", "");
        pathChanged |= DrawConfigString(m_EditorConfig, "assets.cooked_path", "Cooked path", "");
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

void ProjectSettingsPanel::DrawEditorGraphics(EditorContext& context) {
    ImGui::TextUnformatted("Editor Graphics");
    ImGui::Separator();
    if (DrawRendererSettings(m_EditorRenderer)) {
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
    if (DrawLightingShadowSettings(m_EditorRenderer)) {
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
    changed |= DrawConfigBool(m_EditorConfig, "physics.enabled", "Physics enabled", true);
    changed |= DrawPhysicsSettings(m_EditorPhysics, false);
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
    changed |= DrawConfigBool(m_EditorConfig, "gameplay.preview_enabled", "Preview enabled", false);
    changed |= DrawConfigBool(m_EditorConfig, "gameplay.validate_on_load", "Validate on load",
                              context.settings.validateAfterLoad);
    changed |= DrawGameplaySettings(m_EditorGameplay);
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

void ProjectSettingsPanel::DrawClientApplication() {
    ImGui::TextUnformatted("Client Application");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ClientConfig, "app.name", "Application name", "Hockey Game Client");
    changed |= DrawConfigInt(m_ClientConfig, "app.target_fps", "Target FPS", 0, 0, 1000);
    if (changed) {
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientWindowInput() {
    ImGui::TextUnformatted("Client Window / Input");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ClientConfig, "window.title", "Window title", "Hockey Game");
    changed |= DrawConfigInt(m_ClientConfig, "window.width", "Window width", 1920, 320, 7680);
    changed |= DrawConfigInt(m_ClientConfig, "window.height", "Window height", 1080, 240, 4320);
    changed |= DrawConfigBool(m_ClientConfig, "window.resizable", "Resizable", true);
    changed |= DrawConfigBool(m_ClientConfig, "window.maximized", "Maximized", false);
    changed |= DrawConfigBool(m_ClientConfig, "window.start_centered", "Start centered", true);
    changed |= DrawConfigBool(m_ClientConfig, "input.gamepad_enabled", "Gamepad enabled", true);
    changed |= DrawConfigBool(m_ClientConfig, "input.mouse_capture", "Mouse capture", false);
    if (changed) {
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientGraphics() {
    ImGui::TextUnformatted("Client Graphics");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawRendererSettings(m_ClientRenderer)) {
        SaveRendererSettings(m_ClientConfig, m_ClientRenderer);
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientLightingShadows() {
    ImGui::TextUnformatted("Client Lighting & Shadows");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawLightingShadowSettings(m_ClientRenderer)) {
        SaveRendererSettings(m_ClientConfig, m_ClientRenderer);
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientPhysics() {
    ImGui::TextUnformatted("Client Physics");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ClientConfig, "physics.enabled", "Physics enabled", true);
    changed |= DrawPhysicsSettings(m_ClientPhysics, false);
    if (changed) {
        SavePhysicsSettings(m_ClientConfig, m_ClientPhysics);
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientStartupScene() {
    ImGui::TextUnformatted("Client Startup Scene");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawConfigString(m_ClientConfig, "scene.startup_scene", "Startup scene", "")) {
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawClientGameplay() {
    ImGui::TextUnformatted("Client Gameplay");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ClientConfig, "gameplay.local_play", "Local play", true);
    changed |= DrawGameplaySettings(m_ClientGameplay);
    if (changed) {
        SaveGameplaySettings(m_ClientConfig, m_ClientGameplay);
        m_ClientRestartRequired = true;
        SaveClientConfig();
    }
}

void ProjectSettingsPanel::DrawServerApplication() {
    ImGui::TextUnformatted("Server Application");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "app.sleep_when_idle", "Sleep when idle", true);
    changed |= DrawConfigString(m_ServerConfig, "server.name", "Server name", "Local Hockey Server");
    changed |= DrawConfigInt(m_ServerConfig, "server.max_players", "Max players", 8, 1, 128);
    changed |= DrawConfigInt(m_ServerConfig, "server.port", "Port", 27020, 1, 65535);
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerConfig();
    }
}

void ProjectSettingsPanel::DrawServerSimulation() {
    ImGui::TextUnformatted("Server Simulation");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigFloat(m_ServerConfig, "server.tick_rate", "Tick rate", 60.0f, 1.0f, 240.0f);
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerConfig();
    }
}

void ProjectSettingsPanel::DrawServerStartupScene() {
    ImGui::TextUnformatted("Server Startup Scene");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ServerConfig, "scene.startup_scene", "Startup scene", "");
    changed |= DrawConfigBool(m_ServerConfig, "scene.validate_on_load", "Validate on load", true);
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerConfig();
    }
}

void ProjectSettingsPanel::DrawServerPhysics() {
    ImGui::TextUnformatted("Server Physics");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "physics.enabled", "Physics enabled", true);
    changed |= DrawPhysicsSettings(m_ServerPhysics, true);
    if (changed) {
        SavePhysicsSettings(m_ServerConfig, m_ServerPhysics);
        m_ServerRestartRequired = true;
        SaveServerConfig();
    }
}

void ProjectSettingsPanel::DrawServerGameplay() {
    ImGui::TextUnformatted("Server Gameplay");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "gameplay.authoritative", "Authoritative", true);
    changed |= DrawGameplaySettings(m_ServerGameplay);
    if (changed) {
        SaveGameplaySettings(m_ServerConfig, m_ServerGameplay);
        m_ServerRestartRequired = true;
        SaveServerConfig();
    }
}

void ProjectSettingsPanel::DrawPreferences(EditorContext& context) {
    ImGui::TextUnformatted("Editor Preferences");
    ImGui::Separator();
    EditorSettings& s = context.settings;
    bool changed = false;

    if (ImGui::CollapsingHeader("Autosave", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Autosave enabled", &s.autosaveEnabled);
        ImGui::BeginDisabled(!s.autosaveEnabled);
        changed |= ImGui::DragInt("Interval seconds", &s.autosaveIntervalSeconds, 1.0f, 5, 3600);
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Validate before save", &s.validateBeforeSave);
        changed |= ImGui::Checkbox("Validate after load", &s.validateAfterLoad);
    }

    if (ImGui::CollapsingHeader("Grid & Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Show grid", &s.showGrid);
        changed |= ImGui::DragFloat("Grid spacing", &s.gridSpacing, 0.05f, 0.05f, 100.0f, "%.2f");
        changed |= ImGui::Checkbox("Snap enabled", &s.snapEnabled);
        changed |= ImGui::DragFloat("Move snap", &s.moveSnap, 0.01f, 0.001f, 100.0f, "%.3f");
        changed |= ImGui::DragFloat("Rotate snap degrees", &s.rotateSnapDegrees, 0.5f, 1.0f, 180.0f, "%.1f");
        changed |= ImGui::DragFloat("Scale snap", &s.scaleSnap, 0.01f, 0.001f, 100.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat("Move speed", &s.cameraMoveSpeed, 0.1f, 0.1f, 1000.0f, "%.2f");
        changed |= ImGui::DragFloat("Fast multiplier", &s.cameraFastMultiplier, 0.1f, 1.0f, 50.0f, "%.2f");
        changed |= ImGui::DragFloat("Mouse sensitivity", &s.cameraMouseSensitivity, 0.005f, 0.01f, 2.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("Startup", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Restore last scene on launch", &s.restoreLastScene);
    }

    if (ImGui::CollapsingHeader("Asset Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Auto import on change", &s.assetsAutoImport);
        changed |= ImGui::Checkbox("Auto cook dirty assets", &s.assetsAutoCookDirty);
        bool hotReload = s.assetsHotReload;
        if (ImGui::Checkbox("Hot reload", &hotReload)) {
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
    m_Status = "Saved " + m_EditorPath.filename().string();
}

void ProjectSettingsPanel::SaveClientConfig() {
    if (const Status status = m_ClientConfig.Save(m_ClientPath); !status) {
        m_Status = status.error;
        return;
    }
    m_Status = "Saved " + m_ClientPath.filename().string();
}

void ProjectSettingsPanel::SaveServerConfig() {
    if (const Status status = m_ServerConfig.Save(m_ServerPath); !status) {
        m_Status = status.error;
        return;
    }
    m_Status = "Saved " + m_ServerPath.filename().string();
}

void ProjectSettingsPanel::SaveUserPreferences(EditorContext& context) {
    if (const Status status = context.settings.Save(m_UserPreferencesPath); !status) {
        m_Status = status.error;
        return;
    }
    m_Status = "Saved " + m_UserPreferencesPath.filename().string();
}

} // namespace Hockey
