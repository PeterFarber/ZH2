#include "Hockey/Renderer/RendererSettings.hpp"

#include <algorithm>
#include <array>
#include <cctype>

#include "Hockey/Core/Config.hpp"

namespace Hockey {

namespace {

bool IEquals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Generic helper: match text against a {value,name} table.
template <typename Enum, std::size_t N>
bool ParseEnum(std::string_view text, const std::array<std::pair<Enum, const char*>, N>& table, Enum& out) {
    for (const auto& [value, name] : table) {
        if (IEquals(text, name)) {
            out = value;
            return true;
        }
    }
    return false;
}

template <typename Enum, std::size_t N>
const char* StringifyEnum(Enum value, const std::array<std::pair<Enum, const char*>, N>& table) {
    for (const auto& [v, name] : table) {
        if (v == value) {
            return name;
        }
    }
    return "Unknown";
}

constexpr std::array<std::pair<DisplayMode, const char*>, 3> kDisplayModes{{
    {DisplayMode::Windowed, "Windowed"},
    {DisplayMode::BorderlessFullscreen, "BorderlessFullscreen"},
    {DisplayMode::ExclusiveFullscreen, "ExclusiveFullscreen"},
}};

constexpr std::array<std::pair<GraphicsPreset, const char*>, 5> kPresets{{
    {GraphicsPreset::Low, "Low"},
    {GraphicsPreset::Medium, "Medium"},
    {GraphicsPreset::High, "High"},
    {GraphicsPreset::Ultra, "Ultra"},
    {GraphicsPreset::Custom, "Custom"},
}};

constexpr std::array<std::pair<Upscaler, const char*>, 4> kUpscalers{{
    {Upscaler::None, "None"},
    {Upscaler::FSR, "FSR"},
    {Upscaler::XeSS, "XeSS"},
    {Upscaler::DLSS, "DLSS"},
}};

constexpr std::array<std::pair<AntiAliasing, const char*>, 6> kAntiAliasing{{
    {AntiAliasing::Off, "Off"},
    {AntiAliasing::FXAA, "FXAA"},
    {AntiAliasing::TAA, "TAA"},
    {AntiAliasing::MSAA2x, "MSAA2x"},
    {AntiAliasing::MSAA4x, "MSAA4x"},
    {AntiAliasing::MSAA8x, "MSAA8x"},
}};

constexpr std::array<std::pair<TextureQuality, const char*>, 4> kTextureQuality{{
    {TextureQuality::Low, "Low"},
    {TextureQuality::Medium, "Medium"},
    {TextureQuality::High, "High"},
    {TextureQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<ShadowQuality, const char*>, 5> kShadowQuality{{
    {ShadowQuality::Off, "Off"},
    {ShadowQuality::Low, "Low"},
    {ShadowQuality::Medium, "Medium"},
    {ShadowQuality::High, "High"},
    {ShadowQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<AmbientOcclusionQuality, const char*>, 5> kAOQuality{{
    {AmbientOcclusionQuality::Off, "Off"},
    {AmbientOcclusionQuality::Low, "Low"},
    {AmbientOcclusionQuality::Medium, "Medium"},
    {AmbientOcclusionQuality::High, "High"},
    {AmbientOcclusionQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<ReflectionQuality, const char*>, 5> kReflectionQuality{{
    {ReflectionQuality::Off, "Off"},
    {ReflectionQuality::Low, "Low"},
    {ReflectionQuality::Medium, "Medium"},
    {ReflectionQuality::High, "High"},
    {ReflectionQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<EffectQuality, const char*>, 5> kEffectQuality{{
    {EffectQuality::Off, "Off"},
    {EffectQuality::Low, "Low"},
    {EffectQuality::Medium, "Medium"},
    {EffectQuality::High, "High"},
    {EffectQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<DetailQuality, const char*>, 4> kDetailQuality{{
    {DetailQuality::Low, "Low"},
    {DetailQuality::Medium, "Medium"},
    {DetailQuality::High, "High"},
    {DetailQuality::Ultra, "Ultra"},
}};

constexpr std::array<std::pair<ToneMapper, const char*>, 3> kToneMappers{{
    {ToneMapper::Linear, "Linear"},
    {ToneMapper::Reinhard, "Reinhard"},
    {ToneMapper::ACES, "ACES"},
}};

} // namespace

// ---------------------------------------------------------------------------
// Enum <-> string
// ---------------------------------------------------------------------------

const char* ToString(DisplayMode value) {
    return StringifyEnum(value, kDisplayModes);
}
const char* ToString(GraphicsPreset value) {
    return StringifyEnum(value, kPresets);
}
const char* ToString(Upscaler value) {
    return StringifyEnum(value, kUpscalers);
}
const char* ToString(AntiAliasing value) {
    return StringifyEnum(value, kAntiAliasing);
}
const char* ToString(TextureQuality value) {
    return StringifyEnum(value, kTextureQuality);
}
const char* ToString(ShadowQuality value) {
    return StringifyEnum(value, kShadowQuality);
}
const char* ToString(AmbientOcclusionQuality value) {
    return StringifyEnum(value, kAOQuality);
}
const char* ToString(ReflectionQuality value) {
    return StringifyEnum(value, kReflectionQuality);
}
const char* ToString(EffectQuality value) {
    return StringifyEnum(value, kEffectQuality);
}
const char* ToString(DetailQuality value) {
    return StringifyEnum(value, kDetailQuality);
}
const char* ToString(ToneMapper value) {
    return StringifyEnum(value, kToneMappers);
}

bool FromString(std::string_view text, DisplayMode& out) {
    return ParseEnum(text, kDisplayModes, out);
}
bool FromString(std::string_view text, GraphicsPreset& out) {
    return ParseEnum(text, kPresets, out);
}
bool FromString(std::string_view text, Upscaler& out) {
    return ParseEnum(text, kUpscalers, out);
}
bool FromString(std::string_view text, AntiAliasing& out) {
    return ParseEnum(text, kAntiAliasing, out);
}
bool FromString(std::string_view text, TextureQuality& out) {
    return ParseEnum(text, kTextureQuality, out);
}
bool FromString(std::string_view text, ShadowQuality& out) {
    return ParseEnum(text, kShadowQuality, out);
}
bool FromString(std::string_view text, AmbientOcclusionQuality& out) {
    return ParseEnum(text, kAOQuality, out);
}
bool FromString(std::string_view text, ReflectionQuality& out) {
    return ParseEnum(text, kReflectionQuality, out);
}
bool FromString(std::string_view text, EffectQuality& out) {
    return ParseEnum(text, kEffectQuality, out);
}
bool FromString(std::string_view text, DetailQuality& out) {
    return ParseEnum(text, kDetailQuality, out);
}
bool FromString(std::string_view text, ToneMapper& out) {
    return ParseEnum(text, kToneMappers, out);
}

// ---------------------------------------------------------------------------
// Defaults / presets
// ---------------------------------------------------------------------------

RendererSettings MakeDefaultRendererSettings() {
    return RendererSettings{};
}

RendererSettings ApplyGraphicsPreset(GraphicsPreset preset, RendererSettings base) {
    base.preset = preset;
    switch (preset) {
    case GraphicsPreset::Low:
        base.renderScale = 0.75f;
        base.textureQuality = TextureQuality::Low;
        base.anisotropy = 2;
        base.materialQuality = DetailQuality::Low;
        base.shadowQuality = ShadowQuality::Low;
        base.shadowDistance = 50.0f;
        base.contactShadows = false;
        base.aoQuality = AmbientOcclusionQuality::Off;
        base.reflectionQuality = ReflectionQuality::Low;
        base.globalIlluminationQuality = DetailQuality::Low;
        base.modelQuality = DetailQuality::Low;
        base.crowdQuality = DetailQuality::Low;
        base.arenaDetail = DetailQuality::Low;
        base.boardGlassDetail = DetailQuality::Low;
        base.bloom = false;
        base.lensFlare = false;
        base.volumetricLighting = DetailQuality::Low;
        base.particleQuality = DetailQuality::Low;
        base.iceQuality = DetailQuality::Low;
        base.iceReflectionQuality = ReflectionQuality::Low;
        base.iceScratchQuality = DetailQuality::Low;
        base.skateSprayQuality = EffectQuality::Low;
        base.puckTrailQuality = EffectQuality::Low;
        base.jerseyQuality = DetailQuality::Low;
        base.goalNetQuality = DetailQuality::Low;
        base.antiAliasing = AntiAliasing::FXAA;
        base.toneMapper = ToneMapper::Reinhard;
        break;
    case GraphicsPreset::Medium:
        base.renderScale = 1.0f;
        base.textureQuality = TextureQuality::Medium;
        base.anisotropy = 4;
        base.materialQuality = DetailQuality::Medium;
        base.shadowQuality = ShadowQuality::Medium;
        base.shadowDistance = 80.0f;
        base.contactShadows = false;
        base.aoQuality = AmbientOcclusionQuality::Medium;
        base.reflectionQuality = ReflectionQuality::Medium;
        base.globalIlluminationQuality = DetailQuality::Medium;
        base.modelQuality = DetailQuality::Medium;
        base.crowdQuality = DetailQuality::Medium;
        base.arenaDetail = DetailQuality::Medium;
        base.boardGlassDetail = DetailQuality::Medium;
        base.bloom = true;
        base.lensFlare = true;
        base.volumetricLighting = DetailQuality::Medium;
        base.particleQuality = DetailQuality::Medium;
        base.iceQuality = DetailQuality::Medium;
        base.iceReflectionQuality = ReflectionQuality::Medium;
        base.iceScratchQuality = DetailQuality::Medium;
        base.skateSprayQuality = EffectQuality::Medium;
        base.puckTrailQuality = EffectQuality::Medium;
        base.jerseyQuality = DetailQuality::Medium;
        base.goalNetQuality = DetailQuality::Medium;
        base.antiAliasing = AntiAliasing::FXAA;
        base.toneMapper = ToneMapper::ACES;
        break;
    case GraphicsPreset::High:
        base.renderScale = 1.0f;
        base.textureQuality = TextureQuality::High;
        base.anisotropy = 16;
        base.materialQuality = DetailQuality::High;
        base.shadowQuality = ShadowQuality::High;
        base.shadowDistance = 100.0f;
        base.contactShadows = true;
        base.aoQuality = AmbientOcclusionQuality::High;
        base.reflectionQuality = ReflectionQuality::High;
        base.globalIlluminationQuality = DetailQuality::Medium;
        base.modelQuality = DetailQuality::High;
        base.crowdQuality = DetailQuality::Medium;
        base.arenaDetail = DetailQuality::High;
        base.boardGlassDetail = DetailQuality::High;
        base.bloom = true;
        base.lensFlare = true;
        base.volumetricLighting = DetailQuality::Medium;
        base.particleQuality = DetailQuality::High;
        base.iceQuality = DetailQuality::High;
        base.iceReflectionQuality = ReflectionQuality::High;
        base.iceScratchQuality = DetailQuality::High;
        base.skateSprayQuality = EffectQuality::High;
        base.puckTrailQuality = EffectQuality::Medium;
        base.jerseyQuality = DetailQuality::High;
        base.goalNetQuality = DetailQuality::High;
        base.antiAliasing = AntiAliasing::FXAA;
        base.toneMapper = ToneMapper::ACES;
        break;
    case GraphicsPreset::Ultra:
        base.renderScale = 1.0f;
        base.textureQuality = TextureQuality::Ultra;
        base.anisotropy = 16;
        base.materialQuality = DetailQuality::Ultra;
        base.shadowQuality = ShadowQuality::Ultra;
        base.shadowDistance = 150.0f;
        base.contactShadows = true;
        base.aoQuality = AmbientOcclusionQuality::Ultra;
        base.reflectionQuality = ReflectionQuality::Ultra;
        base.globalIlluminationQuality = DetailQuality::High;
        base.modelQuality = DetailQuality::Ultra;
        base.crowdQuality = DetailQuality::High;
        base.arenaDetail = DetailQuality::Ultra;
        base.boardGlassDetail = DetailQuality::Ultra;
        base.bloom = true;
        base.lensFlare = true;
        base.volumetricLighting = DetailQuality::High;
        base.particleQuality = DetailQuality::Ultra;
        base.iceQuality = DetailQuality::Ultra;
        base.iceReflectionQuality = ReflectionQuality::Ultra;
        base.iceScratchQuality = DetailQuality::Ultra;
        base.skateSprayQuality = EffectQuality::Ultra;
        base.puckTrailQuality = EffectQuality::High;
        base.jerseyQuality = DetailQuality::Ultra;
        base.goalNetQuality = DetailQuality::Ultra;
        base.antiAliasing = AntiAliasing::TAA;
        base.toneMapper = ToneMapper::ACES;
        break;
    case GraphicsPreset::Custom:
        // Leave caller-provided base untouched.
        break;
    }
    return base;
}

// ---------------------------------------------------------------------------
// Config persistence. Keys live under the [renderer] table.
// ---------------------------------------------------------------------------

namespace {

template <typename Enum> void LoadEnum(const Config& config, std::string_view key, Enum& field) {
    if (config.Has(key)) {
        Enum parsed{};
        if (FromString(config.GetString(key, ""), parsed)) {
            field = parsed;
        }
    }
}

uint32_t LoadU32(const Config& config, std::string_view key, uint32_t fallback) {
    return static_cast<uint32_t>(config.GetInt(key, static_cast<int>(fallback)));
}

} // namespace

Status LoadRendererSettings(const Config& config, RendererSettings& s) {
    LoadEnum(config, "renderer.display_mode", s.displayMode);
    s.resolutionWidth = LoadU32(config, "renderer.resolution_width", s.resolutionWidth);
    s.resolutionHeight = LoadU32(config, "renderer.resolution_height", s.resolutionHeight);
    s.refreshRate = LoadU32(config, "renderer.refresh_rate", s.refreshRate);
    s.monitorIndex = LoadU32(config, "renderer.monitor_index", s.monitorIndex);
    s.vsync = config.GetBool("renderer.vsync", s.vsync);
    s.fpsLimit = LoadU32(config, "renderer.fps_limit", s.fpsLimit);
    s.hdr = config.GetBool("renderer.hdr", s.hdr);
    s.brightness = static_cast<float>(config.GetDouble("renderer.brightness", s.brightness));
    s.fieldOfView = static_cast<float>(config.GetDouble("renderer.field_of_view", s.fieldOfView));

    s.renderScale = static_cast<float>(config.GetDouble("renderer.render_scale", s.renderScale));
    s.dynamicResolution = config.GetBool("renderer.dynamic_resolution", s.dynamicResolution);
    LoadEnum(config, "renderer.upscaler", s.upscaler);
    s.sharpening = static_cast<float>(config.GetDouble("renderer.sharpening", s.sharpening));

    LoadEnum(config, "renderer.preset", s.preset);

    LoadEnum(config, "renderer.texture_quality", s.textureQuality);
    s.textureStreamingBudgetMB = LoadU32(config, "renderer.texture_streaming_budget_mb", s.textureStreamingBudgetMB);
    s.anisotropy = LoadU32(config, "renderer.anisotropy", s.anisotropy);
    LoadEnum(config, "renderer.material_quality", s.materialQuality);

    LoadEnum(config, "renderer.shadow_quality", s.shadowQuality);
    s.shadowDistance = static_cast<float>(config.GetDouble("renderer.shadow_distance", s.shadowDistance));
    s.contactShadows = config.GetBool("renderer.contact_shadows", s.contactShadows);
    LoadEnum(config, "renderer.ao_quality", s.aoQuality);
    LoadEnum(config, "renderer.reflection_quality", s.reflectionQuality);
    LoadEnum(config, "renderer.global_illumination_quality", s.globalIlluminationQuality);

    LoadEnum(config, "renderer.model_quality", s.modelQuality);
    s.lodDistanceMultiplier =
        static_cast<float>(config.GetDouble("renderer.lod_distance_multiplier", s.lodDistanceMultiplier));
    LoadEnum(config, "renderer.crowd_quality", s.crowdQuality);
    LoadEnum(config, "renderer.arena_detail", s.arenaDetail);
    LoadEnum(config, "renderer.board_glass_detail", s.boardGlassDetail);

    s.bloom = config.GetBool("renderer.bloom", s.bloom);
    s.motionBlur = config.GetBool("renderer.motion_blur", s.motionBlur);
    s.depthOfField = config.GetBool("renderer.depth_of_field", s.depthOfField);
    s.lensFlare = config.GetBool("renderer.lens_flare", s.lensFlare);
    LoadEnum(config, "renderer.volumetric_lighting", s.volumetricLighting);
    LoadEnum(config, "renderer.particle_quality", s.particleQuality);

    LoadEnum(config, "renderer.ice_quality", s.iceQuality);
    LoadEnum(config, "renderer.ice_reflection_quality", s.iceReflectionQuality);
    LoadEnum(config, "renderer.ice_scratch_quality", s.iceScratchQuality);
    LoadEnum(config, "renderer.skate_spray_quality", s.skateSprayQuality);
    LoadEnum(config, "renderer.puck_trail_quality", s.puckTrailQuality);
    LoadEnum(config, "renderer.jersey_quality", s.jerseyQuality);
    LoadEnum(config, "renderer.goal_net_quality", s.goalNetQuality);

    LoadEnum(config, "renderer.anti_aliasing", s.antiAliasing);
    LoadEnum(config, "renderer.tone_mapper", s.toneMapper);
    s.filmGrain = config.GetBool("renderer.film_grain", s.filmGrain);
    s.chromaticAberration = config.GetBool("renderer.chromatic_aberration", s.chromaticAberration);
    s.vignette = config.GetBool("renderer.vignette", s.vignette);

    s.showFPS = config.GetBool("renderer.show_fps", s.showFPS);
    s.showFrameTime = config.GetBool("renderer.show_frame_time", s.showFrameTime);
    s.showGPUStats = config.GetBool("renderer.show_gpu_stats", s.showGPUStats);
    s.showNetworkStats = config.GetBool("renderer.show_network_stats", s.showNetworkStats);

    return Status::Ok();
}

void SaveRendererSettings(Config& config, const RendererSettings& s) {
    config.SetString("renderer.display_mode", ToString(s.displayMode));
    config.SetInt("renderer.resolution_width", static_cast<int>(s.resolutionWidth));
    config.SetInt("renderer.resolution_height", static_cast<int>(s.resolutionHeight));
    config.SetInt("renderer.refresh_rate", static_cast<int>(s.refreshRate));
    config.SetInt("renderer.monitor_index", static_cast<int>(s.monitorIndex));
    config.SetBool("renderer.vsync", s.vsync);
    config.SetInt("renderer.fps_limit", static_cast<int>(s.fpsLimit));
    config.SetBool("renderer.hdr", s.hdr);
    config.SetDouble("renderer.brightness", s.brightness);
    config.SetDouble("renderer.field_of_view", s.fieldOfView);

    config.SetDouble("renderer.render_scale", s.renderScale);
    config.SetBool("renderer.dynamic_resolution", s.dynamicResolution);
    config.SetString("renderer.upscaler", ToString(s.upscaler));
    config.SetDouble("renderer.sharpening", s.sharpening);

    config.SetString("renderer.preset", ToString(s.preset));

    config.SetString("renderer.texture_quality", ToString(s.textureQuality));
    config.SetInt("renderer.texture_streaming_budget_mb", static_cast<int>(s.textureStreamingBudgetMB));
    config.SetInt("renderer.anisotropy", static_cast<int>(s.anisotropy));
    config.SetString("renderer.material_quality", ToString(s.materialQuality));

    config.SetString("renderer.shadow_quality", ToString(s.shadowQuality));
    config.SetDouble("renderer.shadow_distance", s.shadowDistance);
    config.SetBool("renderer.contact_shadows", s.contactShadows);
    config.SetString("renderer.ao_quality", ToString(s.aoQuality));
    config.SetString("renderer.reflection_quality", ToString(s.reflectionQuality));
    config.SetString("renderer.global_illumination_quality", ToString(s.globalIlluminationQuality));

    config.SetString("renderer.model_quality", ToString(s.modelQuality));
    config.SetDouble("renderer.lod_distance_multiplier", s.lodDistanceMultiplier);
    config.SetString("renderer.crowd_quality", ToString(s.crowdQuality));
    config.SetString("renderer.arena_detail", ToString(s.arenaDetail));
    config.SetString("renderer.board_glass_detail", ToString(s.boardGlassDetail));

    config.SetBool("renderer.bloom", s.bloom);
    config.SetBool("renderer.motion_blur", s.motionBlur);
    config.SetBool("renderer.depth_of_field", s.depthOfField);
    config.SetBool("renderer.lens_flare", s.lensFlare);
    config.SetString("renderer.volumetric_lighting", ToString(s.volumetricLighting));
    config.SetString("renderer.particle_quality", ToString(s.particleQuality));

    config.SetString("renderer.ice_quality", ToString(s.iceQuality));
    config.SetString("renderer.ice_reflection_quality", ToString(s.iceReflectionQuality));
    config.SetString("renderer.ice_scratch_quality", ToString(s.iceScratchQuality));
    config.SetString("renderer.skate_spray_quality", ToString(s.skateSprayQuality));
    config.SetString("renderer.puck_trail_quality", ToString(s.puckTrailQuality));
    config.SetString("renderer.jersey_quality", ToString(s.jerseyQuality));
    config.SetString("renderer.goal_net_quality", ToString(s.goalNetQuality));

    config.SetString("renderer.anti_aliasing", ToString(s.antiAliasing));
    config.SetString("renderer.tone_mapper", ToString(s.toneMapper));
    config.SetBool("renderer.film_grain", s.filmGrain);
    config.SetBool("renderer.chromatic_aberration", s.chromaticAberration);
    config.SetBool("renderer.vignette", s.vignette);

    config.SetBool("renderer.show_fps", s.showFPS);
    config.SetBool("renderer.show_frame_time", s.showFrameTime);
    config.SetBool("renderer.show_gpu_stats", s.showGPUStats);
    config.SetBool("renderer.show_network_stats", s.showNetworkStats);
}

} // namespace Hockey
