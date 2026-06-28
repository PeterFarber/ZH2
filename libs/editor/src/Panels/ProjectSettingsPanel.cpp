#include "Hockey/Editor/Panels/ProjectSettingsPanel.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
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
#include "Hockey/Editor/ImGui/EditorTooltip.hpp"
#include "Hockey/Editor/PrefabDragDrop.hpp"
#include "Hockey/Renderer/Renderer.hpp"

namespace Hockey {

namespace {

template <typename T, std::size_t Count>
bool DrawEnumCombo(const char* label, T& value, const std::array<T, Count>& values, const char* tooltip = nullptr) {
    bool changed = false;
    const bool open = ImGui::BeginCombo(label, ToString(value));
    EditorTooltip::ForLastItem(tooltip);
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
    return changed;
}

bool DrawUInt(const char* label, std::uint32_t& value, int minValue, int maxValue, const char* tooltip = nullptr) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    const bool edited = ImGui::DragInt(label, &temp, 1.0f, minValue, maxValue);
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawUIntPowerOfTwoOrZero(const char* label, std::uint32_t& value, int minValue, int maxValue,
                              const char* tooltip = nullptr) {
    int temp = static_cast<int>(std::min<std::uint32_t>(value, static_cast<std::uint32_t>(maxValue)));
    const bool edited = ImGui::DragInt(label, &temp, 64.0f, 0, maxValue);
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        if (temp <= 0) {
            value = 0;
        } else {
            value = static_cast<std::uint32_t>(std::clamp(temp, minValue, maxValue));
        }
        return true;
    }
    return false;
}

bool DrawFloat(const char* label, float& value, float speed, float minValue, float maxValue, const char* format,
               const char* tooltip = nullptr) {
    const bool changed = ImGui::DragFloat(label, &value, speed, minValue, maxValue, format);
    EditorTooltip::ForLastItem(tooltip);
    return changed;
}

bool DrawCheckbox(const char* label, bool& value, const char* tooltip = nullptr) {
    const bool changed = ImGui::Checkbox(label, &value);
    EditorTooltip::ForLastItem(tooltip);
    return changed;
}

bool DrawConfigBool(Config& config, const char* key, const char* label, bool fallback, const char* tooltip = nullptr) {
    bool value = config.GetBool(key, fallback);
    const bool edited = ImGui::Checkbox(label, &value);
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        config.SetBool(key, value);
        return true;
    }
    return false;
}

bool DrawConfigInt(Config& config, const char* key, const char* label, int fallback, int minValue, int maxValue,
                   const char* tooltip = nullptr) {
    int value = config.GetInt(key, fallback);
    const bool edited = ImGui::DragInt(label, &value, 1.0f, minValue, maxValue);
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        config.SetInt(key, std::clamp(value, minValue, maxValue));
        return true;
    }
    return false;
}

bool DrawConfigFloat(Config& config, const char* key, const char* label, float fallback, float minValue,
                     float maxValue, const char* tooltip = nullptr) {
    float value = static_cast<float>(config.GetDouble(key, fallback));
    const bool edited = ImGui::DragFloat(label, &value, 0.05f, minValue, maxValue, "%.3f");
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        config.SetDouble(key, value);
        return true;
    }
    return false;
}

bool DrawConfigString(Config& config, const char* key, const char* label, const char* fallback,
                      const char* tooltip = nullptr) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", config.GetString(key, fallback).c_str());
    const bool edited = ImGui::InputText(label, buffer, sizeof(buffer));
    EditorTooltip::ForLastItem(tooltip);
    if (edited) {
        config.SetString(key, buffer);
        return true;
    }
    return false;
}

bool DrawPrefabPath(const char* label, std::filesystem::path& path, const char* tooltip = nullptr) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", path.generic_string().c_str());

    bool changed = false;
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        path = buffer;
        changed = true;
    }
    EditorTooltip::ForLastItem(tooltip);

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
    EditorTooltip::ForLastItem("Clears the waypoint prefab so gameplay stops spawning visible waypoint markers.");

    return changed;
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
        custom |= DrawEnumCombo("Display mode", settings.displayMode, kDisplayModes,
                                "Controls whether the scene is viewed in a window, borderless desktop surface, or "
                                "exclusive fullscreen output.");
        custom |= DrawUInt("Resolution width", settings.resolutionWidth, 320, 7680,
                           "Backbuffer width in pixels. Higher values make the scene sharper horizontally and cost "
                           "more GPU memory and fill rate.");
        custom |= DrawUInt("Resolution height", settings.resolutionHeight, 240, 4320,
                           "Backbuffer height in pixels. Higher values make the scene sharper vertically and cost "
                           "more GPU memory and fill rate.");
        custom |= DrawUInt("Refresh rate", settings.refreshRate, 0, 480,
                           "Preferred display refresh rate for scene presentation; 0 uses the monitor default.");
        custom |= DrawUInt("Monitor index", settings.monitorIndex, 0, 16,
                           "Selects which monitor shows the scene when fullscreen modes are used.");
        custom |= DrawCheckbox("VSync", settings.vsync,
                               "Synchronizes scene presentation to display refresh to reduce tearing, usually with "
                               "more input latency.");
        custom |= DrawUInt("FPS limit", settings.fpsLimit, 0, 1000,
                           "Caps how often the scene is rendered; 0 leaves it uncapped.");
        custom |= DrawCheckbox("HDR", settings.hdr,
                               "Requests HDR scene rendering and presentation so bright ice highlights and arena "
                               "lights keep more range when supported.");
        custom |= DrawFloat("Brightness", settings.brightness, 0.01f, 0.1f, 4.0f, "%.2f",
                            "Scene brightness multiplier. Higher values lift the whole rendered scene; lower values "
                            "make lighting appear darker.");
        custom |= DrawFloat("Field of view", settings.fieldOfView, 0.1f, 1.0f, 179.0f, "%.1f",
                            "Default camera vertical field of view in degrees. Wider values show more of the rink "
                            "but make scene objects look smaller.");
    }

    if (ImGui::CollapsingHeader("Scaling", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawFloat("Render scale", settings.renderScale, 0.01f, 0.25f, 2.0f, "%.2f",
                            "Internal render resolution multiplier. Changing render scale changes how sharp the scene appears before upscaling; lower values improve performance.");
        custom |= DrawCheckbox("Dynamic resolution", settings.dynamicResolution,
                               "Allows the renderer to lower or raise scene resolution at runtime to protect frame "
                               "rate during heavy lighting or crowd shots.");
        custom |= DrawEnumCombo("Upscaler", settings.upscaler, kUpscalers,
                                "Selects the reconstruction method used when the scene is rendered below output "
                                "resolution.");
        custom |= DrawFloat("Sharpening", settings.sharpening, 0.01f, 0.0f, 2.0f, "%.2f",
                            "Post-upscale sharpening strength. Higher values make boards, jersey numbers, and ice "
                            "scratches crisper but can add ringing.");
    }

    if (ImGui::CollapsingHeader("Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
        GraphicsPreset preset = settings.preset;
        if (DrawEnumCombo("Preset", preset, kPresets,
                          "Applies a bundled graphics quality baseline that changes scene detail, shadows, effects, "
                          "and performance together.")) {
            settings = ApplyGraphicsPreset(preset, settings);
            changed = true;
        }
        custom |= DrawEnumCombo("Texture quality", settings.textureQuality, kTextureQualities,
                                "Controls texture mip detail in the scene. Higher quality keeps jerseys, boards, "
                                "and ice marks sharper at distance.");
        custom |= DrawUInt("Texture budget MB", settings.textureStreamingBudgetMB, 128, 32768,
                           "Approximate texture streaming memory budget. Raising it lets more high-detail scene "
                           "textures stay resident.");
        custom |= DrawUInt("Anisotropy", settings.anisotropy, 1, 16,
                           "Maximum anisotropic filtering level. Higher values sharpen ice, boards, and ads viewed "
                           "at glancing angles.");
        custom |= DrawEnumCombo("Material quality", settings.materialQuality, kDetailQualities,
                                "Controls material feature and shader quality for scene surfaces such as ice, glass, "
                                "jerseys, and metal.");
        custom |= DrawEnumCombo("Model quality", settings.modelQuality, kDetailQualities,
                                "Controls mesh detail targets for scene geometry. Higher quality keeps models more "
                                "detailed farther from the camera.");
        custom |= DrawFloat("LOD distance multiplier", settings.lodDistanceMultiplier, 0.01f, 0.1f, 10.0f, "%.2f",
                            "Scales model LOD switch distances. Higher values keep detailed scene meshes visible "
                            "farther away at a performance cost.");
    }

    if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawCheckbox("Bloom", settings.bloom,
                               "Adds glow around bright scene highlights such as arena lights, ice glare, and "
                               "emissive signage.");
        custom |= DrawCheckbox("Motion blur", settings.motionBlur,
                               "Blurs fast camera or object movement in the scene, making skating and puck motion "
                               "feel smoother but less crisp.");
        custom |= DrawCheckbox("Depth of field", settings.depthOfField,
                               "Adds focus-based background blur so distant scene objects can soften around the "
                               "active camera focus.");
        custom |= DrawCheckbox("Lens flare", settings.lensFlare,
                               "Adds flare artifacts from bright scene lights and reflections.");
        custom |= DrawEnumCombo("Volumetric lighting", settings.volumetricLighting, kDetailQualities,
                                "Controls fog and light-volume quality. Higher values make arena beams and haze "
                                "smoother.");
        custom |= DrawEnumCombo("Particle quality", settings.particleQuality, kDetailQualities,
                                "Controls scene particle detail and count, including skate spray and impact effects.");
        custom |= DrawEnumCombo("Anti-aliasing", settings.antiAliasing, kAntiAliasing,
                                "Selects edge smoothing. Higher-quality modes reduce shimmer on boards, sticks, nets, "
                                "and skaters.");
        custom |= DrawEnumCombo("Tone mapper", settings.toneMapper, kToneMappers,
                                "Chooses how HDR scene lighting is mapped to display colors, affecting contrast and "
                                "highlight rolloff.");
        custom |= DrawCheckbox("Film grain", settings.filmGrain,
                               "Adds a fine grain over the scene for style; disable for the cleanest image.");
        custom |= DrawCheckbox("Chromatic aberration", settings.chromaticAberration,
                               "Adds color fringing near scene image edges; disable for sharper inspection.");
        custom |= DrawCheckbox("Vignette", settings.vignette,
                               "Darkens scene image corners to draw attention toward the rink center.");
    }

    if (ImGui::CollapsingHeader("Hockey", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawEnumCombo("Ice quality", settings.iceQuality, kDetailQualities,
                                "Controls the ice surface shader and detail. Higher values improve gloss, markings, "
                                "and worn-surface response to lights.");
        custom |= DrawEnumCombo("Ice reflection", settings.iceReflectionQuality, kReflectionQualities,
                                "Ice reflection changes how clearly lights, players, and boards reflect on the rink "
                                "surface.");
        custom |= DrawEnumCombo("Ice scratches", settings.iceScratchQuality, kDetailQualities,
                                "Controls scratch and wear detail on the ice so skating paths and surface damage read "
                                "more clearly.");
        custom |= DrawEnumCombo("Skate spray", settings.skateSprayQuality, kEffectQualities,
                                "Controls skate spray particles around hard turns, stops, and collisions.");
        custom |= DrawEnumCombo("Puck trail", settings.puckTrailQuality, kEffectQualities,
                                "Controls puck trail visibility and detail so fast shots are easier to track.");
        custom |= DrawEnumCombo("Jersey quality", settings.jerseyQuality, kDetailQualities,
                                "Controls jersey material and mesh detail for player readability in the scene.");
        custom |= DrawEnumCombo("Goal net quality", settings.goalNetQuality, kDetailQualities,
                                "Controls goal net mesh and material detail, affecting how netting reads near the "
                                "crease.");
        custom |= DrawEnumCombo("Crowd quality", settings.crowdQuality, kDetailQualities,
                                "Controls crowd rendering detail in arena backgrounds.");
        custom |= DrawEnumCombo("Arena detail", settings.arenaDetail, kDetailQualities,
                                "Controls prop and environment detail around the rink.");
        custom |= DrawEnumCombo("Board glass detail", settings.boardGlassDetail, kDetailQualities,
                                "Controls board-glass material and reflection detail around the play surface.");
    }

    if (ImGui::CollapsingHeader("Debug Overlays", ImGuiTreeNodeFlags_DefaultOpen)) {
        custom |= DrawCheckbox("Show FPS", settings.showFPS,
                               "Displays frames per second over the scene while tuning visual settings.");
        custom |= DrawCheckbox("Show frame time", settings.showFrameTime,
                               "Displays frame timing over the scene to spot costly lighting, shadows, or effects.");
        custom |= DrawCheckbox("Show GPU stats", settings.showGPUStats,
                               "Displays renderer GPU statistics over the scene for graphics tuning.");
        custom |= DrawCheckbox("Show network stats", settings.showNetworkStats,
                               "Displays network statistics over the client scene when network overlays are active.");
    }

    if (custom) {
        settings.preset = GraphicsPreset::Custom;
    }
    return changed || custom;
}

bool DrawLightingShadowSettings(RendererSettings& settings) {
    bool changed = false;

    if (ImGui::CollapsingHeader("Budgets", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Budgets");
        changed |= DrawUInt("Max rendered lights", settings.maxRenderedLights, 0, 16,
                            "Caps how many scene lights the renderer submits. Lower values improve performance but "
                            "may leave lower-priority lights unrendered.");
        changed |= DrawUInt("Max local shadow tiles", settings.maxLocalShadowTiles, 0, 16,
                            "Caps shadow atlas tiles for point and spot lights. More tiles let more local scene "
                            "lights cast shadows at the same time.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Shadow Atlases", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Shadow Atlases");
        changed |= DrawEnumCombo("Shadow quality", settings.shadowQuality,
                                 std::array{ShadowQuality::Off, ShadowQuality::Low, ShadowQuality::Medium,
                                            ShadowQuality::High, ShadowQuality::Ultra},
                                 "Overall shadow quality preset. Higher settings improve scene shadow sharpness, "
                                 "distance, and filtering at a GPU cost.");
        changed |= DrawFloat("Shadow distance", settings.shadowDistance, 1.0f, 0.0f, 5000.0f, "%.0f",
                             "Maximum distance for shadow rendering. Raising it lets rink, player, and arena shadows "
                             "remain visible farther from the camera.");
        changed |= DrawUIntPowerOfTwoOrZero("Directional atlas", settings.directionalShadowAtlasResolution, 256, 16384,
                                            "Directional shadow atlas resolution. Directional atlas controls how much texel detail the sun shadow map has; 0 uses the preset default.");
        changed |= DrawUIntPowerOfTwoOrZero("Local atlas", settings.localShadowAtlasResolution, 256, 16384,
                                            "Local light shadow atlas resolution. Higher values sharpen point and spot "
                                            "light shadows from arena fixtures; 0 uses the preset default.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Directional Cascades", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Directional Cascades");
        changed |= DrawUInt("Cascade count", settings.shadowCascadeCount, 0, 4,
                            "Cascade count controls how far directional shadows stay detailed across the camera view; "
                            "0 uses the preset default.");
        changed |= DrawFloat("Split lambda", settings.shadowCascadeSplitLambda, 0.01f, 0.0f, 1.0f, "%.2f",
                             "Controls where cascade resolution is spent. Higher values reserve more directional "
                             "shadow detail near the camera.");
        changed |= DrawFloat("Overlap scale", settings.shadowCascadeOverlapScale, 0.01f, 0.0f, 1.0f, "%.2f",
                             "Scales overlap between neighboring cascades to reduce visible shadow seams while the "
                             "camera moves through the scene.");
        changed |= DrawFloat("Minimum overlap", settings.shadowCascadeMinOverlap, 0.25f, 0.0f, 100.0f, "%.2f",
                             "Minimum cascade overlap distance. Higher values make cascade transitions safer for "
                             "large open rink views.");
        changed |= DrawFloat("Max overlap scale", settings.shadowCascadeMaxOverlapScale, 0.01f, 0.0f, 1.0f,
                             "%.2f", "Upper bound for cascade overlap scaling so distant scene shadows do not spend "
                                      "too much range on transitions.");
        changed |= DrawFloat("Blend scale", settings.shadowCascadeBlendScale, 0.01f, 0.0f, 1.0f, "%.2f",
                             "Scales the soft blend region between cascades. Higher values hide cascade transitions "
                             "but soften shadows across the blend.");
        changed |= DrawFloat("Minimum blend", settings.shadowCascadeMinBlend, 0.05f, 0.0f, 50.0f, "%.2f",
                             "Minimum distance used for cascade blending so nearby scene shadows do not switch "
                             "abruptly.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Directional Filter & Bias", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Directional Filter & Bias");
        changed |= DrawUInt("PCF radius", settings.directionalShadowPcfRadius, 0, 3,
                            "Directional shadow filter radius. Higher values soften sun shadows from players, sticks, "
                            "and rink props.");
        changed |= DrawFloat("Normal offset scale", settings.directionalShadowNormalOffsetScale, 0.01f, 0.0f,
                             4.0f, "%.2f", "Scales normal-based directional shadow offset to reduce acne on lit "
                                             "scene surfaces.");
        changed |= DrawFloat("Normal offset min", settings.directionalShadowNormalOffsetMin, 0.0005f, 0.0f,
                             0.25f, "%.4f", "Minimum normal offset for directional shadows so flat surfaces like ice "
                                             "avoid self-shadow speckling.");
        changed |= DrawFloat("Normal offset max", settings.directionalShadowNormalOffsetMax, 0.001f, 0.0f, 1.0f,
                             "%.4f", "Maximum normal offset for directional shadows. Too high can detach shadows from "
                                      "skaters and boards.");
        changed |= DrawFloat("Bias base", settings.directionalShadowBiasBase, 0.00005f, 0.0f, 0.02f, "%.5f",
                             "Base depth bias for directional shadows. Raising it reduces acne but can make shadows "
                             "float away from scene objects.");
        changed |= DrawFloat("Bias slope", settings.directionalShadowBiasSlope, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Slope-scaled depth bias for directional shadows on angled scene surfaces.");
        changed |= DrawFloat("Bias min", settings.directionalShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f",
                             "Minimum computed directional shadow bias applied to keep close-up shadows stable.");
        changed |= DrawFloat("Bias max", settings.directionalShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Maximum computed directional shadow bias. Lower values keep contact tighter but risk "
                             "shadow acne.");
        changed |= DrawFloat("Depth bias constant", settings.directionalShadowDepthBiasConstant, 0.05f, 0.0f,
                             8.0f, "%.2f", "Rasterizer constant depth bias for directional shadows during shadow-map "
                                             "rendering.");
        changed |= DrawFloat("Depth bias slope", settings.directionalShadowDepthBiasSlope, 0.05f, 0.0f, 8.0f,
                             "%.2f", "Rasterizer slope depth bias for directional shadow casters on angled geometry.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Contact Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Contact Shadows");
        changed |= DrawCheckbox("Contact shadows", settings.contactShadows,
                               "Contact shadows add small grounding shadows under skates, puck, sticks, and props "
                               "where shadow maps are too coarse.");
        changed |= DrawFloat("Distance", settings.contactShadowDistance, 0.5f, 0.0f, 500.0f, "%.1f",
                             "Maximum contact shadow ray distance. Higher values extend fine grounding shadows around "
                             "nearby scene objects.");
        changed |= DrawFloat("Strength", settings.contactShadowStrength, 0.01f, 0.0f, 1.0f, "%.2f",
                             "Contact shadow opacity multiplier. Higher values make contact points darker and more "
                             "anchored.");
        changed |= DrawFloat("Normal offset scale", settings.contactShadowNormalOffsetScale, 0.01f, 0.0f, 2.0f,
                             "%.2f", "Scales normal-based contact shadow offset to avoid self-shadow speckles on "
                                      "close surfaces.");
        changed |= DrawFloat("Normal offset min", settings.contactShadowNormalOffsetMin, 0.0005f, 0.0f, 0.25f,
                             "%.4f", "Minimum normal offset for contact shadows on flat scene surfaces.");
        changed |= DrawFloat("Bias base", settings.contactShadowBiasBase, 0.00005f, 0.0f, 0.02f, "%.5f",
                             "Base depth bias for contact shadows. Raising it reduces artifacts but can weaken tiny "
                             "grounding shadows.");
        changed |= DrawFloat("Bias slope", settings.contactShadowBiasSlope, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Slope-scaled depth bias for contact shadows on angled geometry.");
        changed |= DrawFloat("Bias min", settings.contactShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f",
                             "Minimum computed contact shadow bias used for stable close-up grounding.");
        changed |= DrawFloat("Bias max", settings.contactShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Maximum computed contact shadow bias. Lower values keep contacts tight but can add "
                             "speckling.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Local Light Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Local Light Shadows");
        changed |= DrawUInt("PCF radius", settings.localShadowPcfRadius, 0, 3,
                            "Local light shadow filter radius. Higher values soften point and spot shadows from arena "
                            "fixtures.");
        changed |= DrawFloat("Bias scale", settings.localShadowBiasScale, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Scales depth bias for local light shadows. Raising it reduces acne but can detach "
                             "shadows from props.");
        changed |= DrawFloat("Bias min", settings.localShadowBiasMin, 0.00005f, 0.0f, 0.02f, "%.5f",
                             "Minimum local light shadow bias for stable close local shadows.");
        changed |= DrawFloat("Bias max", settings.localShadowBiasMax, 0.00005f, 0.0f, 0.05f, "%.5f",
                             "Maximum local light shadow bias. Lower values keep point and spot shadows tighter.");
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Ambient & Reflections", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Ambient & Reflections");
        changed |= DrawEnumCombo("Ambient occlusion", settings.aoQuality,
                                 std::array{AmbientOcclusionQuality::Off, AmbientOcclusionQuality::Low,
                                            AmbientOcclusionQuality::Medium, AmbientOcclusionQuality::High,
                                            AmbientOcclusionQuality::Ultra},
                                 "Screen-space ambient occlusion quality. Higher values darken creases and contact "
                                 "areas around skates, boards, nets, and props.");
        changed |= DrawEnumCombo("Reflection quality", settings.reflectionQuality,
                                 std::array{ReflectionQuality::Off, ReflectionQuality::Low,
                                            ReflectionQuality::Medium, ReflectionQuality::High,
                                            ReflectionQuality::Ultra},
                                 "Reflection probe and screen reflection quality. Higher values improve reflected "
                                 "arena lights and nearby scene objects.");
        changed |= DrawEnumCombo("Global illumination", settings.globalIlluminationQuality,
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

bool DrawPhysicsSettings(PhysicsSettings& settings, bool serverMode) {
    bool changed = false;
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, 0.0005f, 0.001f, 0.1f, "%.4f",
                         "Physics fixed delta controls how often preview collisions, skating contacts, and puck "
                         "movement are simulated.");
    changed |= DrawUInt("Max bodies", settings.maxBodies, 1, 1000000,
                        "Maximum physics bodies allowed in the scene before new dynamic objects fail to register.");
    changed |= DrawUInt("Max body pairs", settings.maxBodyPairs, 1, 1000000,
                        "Maximum broadphase body pairs. Higher values support denser scenes with more possible "
                        "collisions.");
    changed |= DrawUInt("Max contact constraints", settings.maxContactConstraints, 1, 1000000,
                        "Maximum active contact constraints. Raise this for scenes with many simultaneous body, puck, "
                        "board, and prop contacts.");
    changed |= DrawUInt("Temp allocator MB", settings.tempAllocatorSizeMB, 1, 4096,
                        "Temporary physics memory used while solving scene contacts and simulation islands.");
    changed |= DrawUInt("Job thread count", settings.jobThreadCount, 0, 128,
                        "Physics worker thread count. 0 lets the engine choose based on hardware.");
    changed |= DrawFloat("Gravity X", settings.gravity.x, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Horizontal gravity applied to dynamic scene bodies.");
    changed |= DrawFloat("Gravity Y", settings.gravity.y, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Vertical gravity applied to dynamic scene bodies, including airborne puck and prop motion.");
    changed |= DrawFloat("Gravity Z", settings.gravity.z, 0.01f, -100.0f, 100.0f, "%.2f",
                         "Depth-axis gravity applied to dynamic scene bodies.");
    changed |= DrawUInt("Collision steps", settings.collisionSteps, 1, 32,
                        "Collision solver steps per fixed update. Higher values improve contact stability in busy "
                        "scenes.");
    changed |= DrawUInt("Integration substeps", settings.integrationSubsteps, 1, 32,
                        "Subdivides each fixed update. Higher values make fast puck and player contacts more stable "
                        "at a CPU cost.");
    if (serverMode) {
        changed |= DrawCheckbox("Deterministic mode", settings.deterministicMode,
                                "Keeps authoritative scene simulation reproducible for server snapshots and replay.");
    }
    changed |= DrawCheckbox("Enable sleeping", settings.enableSleeping,
                            "Lets inactive scene bodies sleep to save CPU until contact or force wakes them.");
    changed |= DrawCheckbox("Enable debug draw", settings.enableDebugDraw,
                            "Draws physics shapes and contacts over the scene for collider inspection.");
    changed |= DrawFloat("World min Y", settings.worldMinY, 1.0f, -100000.0f, 0.0f, "%.0f",
                         "Vertical kill plane for physics bodies that fall out of the scene.");
    return changed;
}

bool DrawGameplaySettings(GameplaySettings& settings) {
    bool changed = false;
    changed |= DrawCheckbox("Enabled", settings.enabled,
                            "Enables hockey gameplay systems for the scene, including puck, teams, scoring, and game "
                            "flow.");
    changed |= DrawUInt("Target player count", settings.targetPlayerCount, 1, 64,
                        "Target player count controls how many skaters and goalies the scene simulation expects.");
    changed |= DrawUInt("Skaters per team", settings.skatersPerTeam, 0, 16,
                        "Number of skater spawn slots expected for each team in gameplay scenes.");
    changed |= DrawUInt("Goalies per team", settings.goaliesPerTeam, 0, 4,
                        "Number of goalie spawn slots expected for each team in gameplay scenes.");
    changed |= DrawFloat("Fixed delta seconds", settings.fixedDeltaSeconds, 0.0005f, 0.001f, 0.1f, "%.4f",
                         "Gameplay fixed delta controls how often rules, possession, scoring, and preview input tick.");
    changed |= DrawFloat("Period length seconds", settings.periodLengthSeconds, 1.0f, 1.0f, 7200.0f, "%.0f",
                         "Length of each gameplay period in seconds for the scene's match flow.");
    changed |= DrawUInt("Period count", settings.periodCount, 1, 12,
                        "Number of periods the scene's match flow plays before final game state.");
    changed |= DrawCheckbox("Stop clock after goal", settings.stopClockAfterGoal,
                            "Stops the game clock after goals so the scene can reset for celebration and faceoff.");
    changed |= DrawCheckbox("Auto faceoff after goal", settings.autoFaceoffAfterGoal,
                            "Automatically returns players and puck to faceoff positions after a goal.");
    changed |= DrawFloat("Post-goal delay seconds", settings.postGoalDelaySeconds, 0.1f, 0.0f, 60.0f, "%.1f",
                         "Delay before the scene resets to faceoff after a goal.");
    changed |= DrawFloat("Faceoff delay seconds", settings.faceoffDelaySeconds, 0.1f, 0.0f, 60.0f, "%.1f",
                         "Delay between faceoff setup and live play, keeping players and puck staged before the drop.");
    changed |= DrawFloat("Goal detection radius", settings.goalDetectionRadius, 0.01f, 0.0f, 20.0f, "%.2f",
                         "Distance from the goal marker where puck position scoring is accepted.");
    changed |= DrawCheckbox("Require puck for goal", settings.requirePuckForGoal,
                            "Requires goal trigger scoring to come from a puck entity rather than any object.");
    changed |= DrawCheckbox("Allow manual goalie", settings.allowManualGoalie,
                            "Allows a player-controlled goalie role when the scene has goalie spawns.");
    changed |= DrawCheckbox("Allow out of play", settings.allowOutOfPlay,
                            "Allows puck and player state to enter out-of-play handling when leaving legal areas.");
    changed |= DrawPrefabPath("Waypoint prefab", settings.waypointPrefabPath,
                              "Prefab instantiated for visible click-to-move waypoint markers. Drag a prefab from "
                              "the Project panel or type a project-relative .prefab.yaml path.");
    changed |= DrawCheckbox("Debug draw gameplay", settings.debugDrawGameplay,
                            "Draws gameplay markers, zones, and role debug information over the scene.");
    changed |= DrawCheckbox("Log gameplay events", settings.logGameplayEvents,
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
    ImGui::TextUnformatted("Client Build Defaults");
    ImGui::PushID("Client");
    item("Application Defaults", Section::ClientApplication);
    item("Window / Input Defaults", Section::ClientWindowInput);
    item("Graphics Defaults", Section::ClientGraphics);
    item("Lighting & Shadows", Section::ClientLightingShadows);
    item("Physics", Section::ClientPhysics);
    item("Gameplay", Section::ClientGameplay);
    item("Startup Scene", Section::ClientStartupScene);
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
    restart |= DrawConfigString(m_EditorConfig, "app.name", "Application name", "Hockey Map Editor",
                                "Name used by the editor process while authoring and previewing scenes.");
    restart |= DrawConfigInt(m_EditorConfig, "app.target_fps", "Target FPS", 0, 0, 1000,
                             "Caps editor update cadence while inspecting scenes; 0 leaves the editor uncapped.");
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
        changed |= DrawConfigString(m_EditorConfig, "window.title", "Title", "Hockey Map Editor",
                                    "Title shown on the editor window used to view and edit scenes.");
        changed |= DrawConfigInt(m_EditorConfig, "window.width", "Width", 1600, 320, 7680,
                                 "Initial editor window width. Larger values show more of the scene and panels at "
                                 "startup.");
        changed |= DrawConfigInt(m_EditorConfig, "window.height", "Height", 900, 240, 4320,
                                 "Initial editor window height. Larger values show more scene viewport area at "
                                 "startup.");
        changed |= DrawConfigBool(m_EditorConfig, "window.resizable", "Resizable", true,
                                  "Allows resizing the editor window while inspecting the scene layout.");
        changed |= DrawConfigBool(m_EditorConfig, "window.maximized", "Maximized", true,
                                  "Starts the editor maximized so the scene and tool panels get the largest workspace.");
        changed |= DrawConfigBool(m_EditorConfig, "window.start_centered", "Start centered", true,
                                  "Centers the editor window before loading the scene workspace.");
        if (changed) {
            m_EditorRestartRequired = true;
            SaveEditorConfig(context);
        }
    }

    if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool gamepad = m_EditorConfig.GetBool("input.gamepad_enabled", true);
        const bool gamepadEdited = ImGui::Checkbox("Gamepad enabled", &gamepad);
        EditorTooltip::ForLastItem("Allows gamepad input while previewing or testing scene interactions in the editor.");
        if (gamepadEdited) {
            m_EditorConfig.SetBool("input.gamepad_enabled", gamepad);
            Input::SetGamepadEnabled(gamepad);
            SaveEditorConfig(context);
        }
        if (DrawConfigBool(m_EditorConfig, "input.mouse_capture", "Mouse capture", false,
                           "Captures the mouse for editor camera or preview controls when scene interaction needs "
                           "relative input.")) {
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
                                  context.settings.autosaveEnabled,
                                  "Writes backup scene files automatically so authoring changes are recoverable.");
        changed |= DrawConfigInt(m_EditorConfig, "scene.autosave_interval_seconds", "Autosave interval seconds",
                                 context.settings.autosaveIntervalSeconds, 5, 3600,
                                 "Time between automatic scene backups while editing.");
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
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_discover", "Auto discover", false,
                                  "Scans asset folders for files that can be used by scenes.");
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_import", "Auto import", false,
                                  "Imports newly discovered source assets so scenes can reference them without manual "
                                  "steps.");
        changed |= DrawConfigBool(m_EditorConfig, "assets.auto_cook_dirty", "Auto cook dirty", false,
                                  "Rebuilds changed cooked assets so scene previews use current meshes, materials, "
                                  "and textures.");
        bool hotReload = m_EditorConfig.GetBool("assets.hot_reload", false);
        const bool hotReloadEdited = ImGui::Checkbox("Hot reload", &hotReload);
        EditorTooltip::ForLastItem("Hot reload updates loaded scene assets when files change on disk.");
        if (hotReloadEdited) {
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
        pathChanged |= DrawConfigString(m_EditorConfig, "assets.raw_path", "Raw path", "",
                                        "Source asset folder used for scene-importable files.");
        pathChanged |= DrawConfigString(m_EditorConfig, "assets.cooked_path", "Cooked path", "",
                                        "Cooked asset folder loaded by scene previews and runtime builds.");
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
    changed |= DrawConfigBool(m_EditorConfig, "physics.enabled", "Physics enabled", true,
                              "Enables physics simulation for editor scene preview.");
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
    changed |= DrawConfigBool(m_EditorConfig, "gameplay.preview_enabled", "Preview enabled", false,
                              "Enables gameplay preview so the authored scene can run hockey rules in the editor.");
    changed |= DrawConfigBool(m_EditorConfig, "gameplay.validate_on_load", "Validate on load",
                              context.settings.validateAfterLoad,
                              "Validate on load checks the scene for missing required gameplay objects before play.");
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
    ImGui::TextUnformatted("Client Build Defaults");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ClientConfig, "app.name", "Application name", "Hockey Game Client",
                                "Name used by the playable client build that loads game scenes.");
    changed |= DrawConfigInt(m_ClientConfig, "app.target_fps", "Target FPS", 0, 0, 1000,
                             "Caps the playable scene frame rate; 0 leaves it uncapped.");
    if (changed) {
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientWindowInput() {
    ImGui::TextUnformatted("Client Window / Input Defaults");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ClientConfig, "window.title", "Window title", "Hockey Game",
                                "Title shown on the playable client window while rendering the scene.");
    changed |= DrawConfigInt(m_ClientConfig, "window.width", "Window width", 1920, 320, 7680,
                             "Initial client scene width in pixels.");
    changed |= DrawConfigInt(m_ClientConfig, "window.height", "Window height", 1080, 240, 4320,
                             "Initial client scene height in pixels.");
    changed |= DrawConfigBool(m_ClientConfig, "window.resizable", "Resizable", true,
                              "Allows players to resize the game window and change the visible scene area.");
    changed |= DrawConfigBool(m_ClientConfig, "window.maximized", "Maximized", false,
                              "Starts the playable client maximized for a larger scene view.");
    changed |= DrawConfigBool(m_ClientConfig, "window.start_centered", "Start centered", true,
                              "Centers the playable client window before the startup scene appears.");
    changed |= DrawConfigBool(m_ClientConfig, "input.gamepad_enabled", "Gamepad enabled", true,
                              "Allows gamepad input to control players in the scene.");
    changed |= DrawConfigBool(m_ClientConfig, "input.mouse_capture", "Mouse capture", false,
                              "Captures the mouse for gameplay or camera controls that need relative input.");
    if (changed) {
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientGraphics() {
    ImGui::TextUnformatted("Client Graphics Defaults");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawRendererSettings(m_ClientRenderer)) {
        SaveRendererSettings(m_ClientConfig, m_ClientRenderer);
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientLightingShadows() {
    ImGui::TextUnformatted("Client Lighting & Shadows");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawLightingShadowSettings(m_ClientRenderer)) {
        SaveRendererSettings(m_ClientConfig, m_ClientRenderer);
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientPhysics() {
    ImGui::TextUnformatted("Client Physics");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ClientConfig, "physics.enabled", "Physics enabled", true,
                              "Enables client-side physics simulation for scene prediction and local preview.");
    changed |= DrawPhysicsSettings(m_ClientPhysics, false);
    if (changed) {
        SavePhysicsSettings(m_ClientConfig, m_ClientPhysics);
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientStartupScene() {
    ImGui::TextUnformatted("Client Startup Scene");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    if (DrawConfigString(m_ClientConfig, "scene.startup_scene", "Startup scene", "",
                         "Startup scene is the scene loaded when players launch the build.")) {
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawClientGameplay() {
    ImGui::TextUnformatted("Client Gameplay");
    DrawRestartBadge(m_ClientRestartRequired, "Client relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ClientConfig, "gameplay.local_play", "Local play", true,
                              "Runs gameplay locally in the client scene for offline testing.");
    changed |= DrawGameplaySettings(m_ClientGameplay);
    if (changed) {
        SaveGameplaySettings(m_ClientConfig, m_ClientGameplay);
        m_ClientRestartRequired = true;
        SaveClientBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawServerApplication() {
    ImGui::TextUnformatted("Server Build Defaults");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "app.sleep_when_idle", "Sleep when idle", true,
                              "Lets the headless server sleep when no scene simulation work is active.");
    changed |= DrawConfigString(m_ServerConfig, "server.name", "Server name", "Local Hockey Server",
                                "Name advertised for the server that owns the authoritative scene.");
    changed |= DrawConfigInt(m_ServerConfig, "server.max_players", "Max players", 8, 1, 128,
                             "Maximum connected players allowed to join the authoritative scene.");
    changed |= DrawConfigInt(m_ServerConfig, "server.port", "Port", 27020, 1, 65535,
                             "Network port clients use to connect to the authoritative scene server.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawServerSimulation() {
    ImGui::TextUnformatted("Server Simulation Defaults");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigFloat(m_ServerConfig, "server.tick_rate", "Tick rate", 60.0f, 1.0f, 240.0f,
                               "Authoritative scene simulation ticks per second. Higher values improve responsiveness "
                               "and cost more CPU/network budget.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawServerStartupScene() {
    ImGui::TextUnformatted("Server Startup Scene");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigString(m_ServerConfig, "scene.startup_scene", "Startup scene", "",
                                "Scene loaded by the headless server when it starts authoritative simulation.");
    changed |= DrawConfigBool(m_ServerConfig, "scene.validate_on_load", "Validate on load", true,
                              "Checks the startup scene for required hockey gameplay objects before the server runs.");
    if (changed) {
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawServerPhysics() {
    ImGui::TextUnformatted("Server Physics");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "physics.enabled", "Physics enabled", true,
                              "Enables authoritative physics simulation for the server scene.");
    changed |= DrawPhysicsSettings(m_ServerPhysics, true);
    if (changed) {
        SavePhysicsSettings(m_ServerConfig, m_ServerPhysics);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawServerGameplay() {
    ImGui::TextUnformatted("Server Gameplay");
    DrawRestartBadge(m_ServerRestartRequired, "Server relaunch required");
    ImGui::Separator();
    bool changed = false;
    changed |= DrawConfigBool(m_ServerConfig, "gameplay.authoritative", "Authoritative", true,
                              "Makes the server own gameplay truth for the scene, including scoring, rules, and "
                              "snapshots.");
    changed |= DrawGameplaySettings(m_ServerGameplay);
    if (changed) {
        SaveGameplaySettings(m_ServerConfig, m_ServerGameplay);
        m_ServerRestartRequired = true;
        SaveServerBuildDefaults();
    }
}

void ProjectSettingsPanel::DrawPreferences(EditorContext& context) {
    ImGui::TextUnformatted("Editor Preferences");
    ImGui::Separator();
    EditorSettings& s = context.settings;
    bool changed = false;

    if (ImGui::CollapsingHeader("Autosave", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Autosave enabled", s.autosaveEnabled,
                                "Automatically saves backup copies of the open scene while editing.");
        ImGui::BeginDisabled(!s.autosaveEnabled);
        changed |= ImGui::DragInt("Interval seconds", &s.autosaveIntervalSeconds, 1.0f, 5, 3600);
        EditorTooltip::ForLastItem("How often the editor writes scene autosave files.");
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Validate before save", s.validateBeforeSave,
                                "Checks the scene for missing hockey gameplay objects before writing it to disk.");
        changed |= DrawCheckbox("Validate after load", s.validateAfterLoad,
                                "Checks the scene after opening it so missing puck, rink, goal, and spawn markers are "
                                "reported immediately.");
    }

    if (ImGui::CollapsingHeader("Grid & Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Show grid", s.showGrid,
                                "Shows the ground grid in the scene viewport for rink and object alignment.");
        changed |= DrawFloat("Grid spacing", s.gridSpacing, 0.05f, 0.05f, 100.0f, "%.2f",
                             "Distance between scene viewport grid lines.");
        changed |= DrawCheckbox("Snap enabled", s.snapEnabled,
                                "Constrains transform edits to scene-friendly increments for cleaner placement.");
        changed |= DrawFloat("Move snap", s.moveSnap, 0.01f, 0.001f, 100.0f, "%.3f",
                             "World-unit increment used when moving scene objects with snapping enabled.");
        changed |= DrawFloat("Rotate snap degrees", s.rotateSnapDegrees, 0.5f, 1.0f, 180.0f, "%.1f",
                             "Degree increment used when rotating scene objects with snapping enabled.");
        changed |= DrawFloat("Scale snap", s.scaleSnap, 0.01f, 0.001f, 100.0f, "%.3f",
                             "Scale increment used when resizing scene objects with snapping enabled.");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawFloat("Move speed", s.cameraMoveSpeed, 0.1f, 0.1f, 1000.0f, "%.2f",
                             "Base speed for flying the editor camera through the scene.");
        changed |= DrawFloat("Fast multiplier", s.cameraFastMultiplier, 0.1f, 1.0f, 50.0f, "%.2f",
                             "Multiplier applied to editor camera movement for crossing large scenes quickly.");
        changed |= DrawFloat("Mouse sensitivity", s.cameraMouseSensitivity, 0.005f, 0.01f, 2.0f, "%.3f",
                             "Mouse-look sensitivity for orbiting or flying the scene camera.");
    }

    if (ImGui::CollapsingHeader("Startup", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Restore last scene on launch", s.restoreLastScene,
                                "Reopens the last edited scene when the editor starts.");
    }

    if (ImGui::CollapsingHeader("Asset Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= DrawCheckbox("Auto import on change", s.assetsAutoImport,
                                "Imports changed source assets so scene references can refresh without manual import.");
        changed |= DrawCheckbox("Auto cook dirty assets", s.assetsAutoCookDirty,
                                "Cooks modified assets so scene previews use current runtime-ready data.");
        bool hotReload = s.assetsHotReload;
        const bool hotReloadEdited = ImGui::Checkbox("Hot reload", &hotReload);
        EditorTooltip::ForLastItem("Reloads changed assets into open scenes when the asset watcher detects updates.");
        if (hotReloadEdited) {
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

void ProjectSettingsPanel::SaveClientBuildDefaults() {
    if (const Status status = m_ClientConfig.Save(m_ClientPath); !status) {
        m_Status = status.error;
        return;
    }
    m_Status = "Saved " + m_ClientPath.filename().string();
}

void ProjectSettingsPanel::SaveServerBuildDefaults() {
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
