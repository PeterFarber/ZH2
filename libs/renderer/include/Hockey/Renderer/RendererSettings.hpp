#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

class Config;

// ---------------------------------------------------------------------------
// Graphics settings enums
// ---------------------------------------------------------------------------

enum class DisplayMode { Windowed, BorderlessFullscreen, ExclusiveFullscreen };
enum class GraphicsPreset { Low, Medium, High, Ultra, Custom };
enum class Upscaler { None, FSR, XeSS, DLSS };
enum class AntiAliasing { Off, FXAA, TAA, MSAA2x, MSAA4x, MSAA8x };
enum class TextureQuality { Low, Medium, High, Ultra };
enum class ShadowQuality { Off, Low, Medium, High, Ultra };
enum class AmbientOcclusionQuality { Off, Low, Medium, High, Ultra };
enum class ReflectionQuality { Off, Low, Medium, High, Ultra };
enum class EffectQuality { Off, Low, Medium, High, Ultra };
enum class DetailQuality { Low, Medium, High, Ultra };
enum class ToneMapper { Linear, Reinhard, ACES };

inline constexpr uint32_t kRendererMaxLights = 16;
inline constexpr uint32_t kRendererMaxCascades = 4;
inline constexpr uint32_t kRendererMaxLocalShadowTiles = 16;

// ---------------------------------------------------------------------------
// Complete renderer settings model. Fields cover display, upscaling, quality,
// lighting, geometry, effects, hockey-specific quality, post-processing and
// debug/performance flags. Only a subset of these is wired into actual GPU
// behavior in Phase 3; the rest are persisted data paths for later phases.
// ---------------------------------------------------------------------------

struct RendererSettings {
    // Display
    DisplayMode displayMode = DisplayMode::Windowed;
    uint32_t resolutionWidth = 1920;
    uint32_t resolutionHeight = 1080;
    uint32_t refreshRate = 0;
    uint32_t monitorIndex = 0;
    bool vsync = true;
    uint32_t fpsLimit = 0;
    bool hdr = true;
    float brightness = 1.0f;
    float fieldOfView = 70.0f;

    // Upscaling / scaling
    float renderScale = 1.0f;
    bool dynamicResolution = false;
    Upscaler upscaler = Upscaler::None;
    float sharpening = 0.3f;

    // Preset
    GraphicsPreset preset = GraphicsPreset::High;

    // Textures / materials
    TextureQuality textureQuality = TextureQuality::High;
    uint32_t textureStreamingBudgetMB = 4096;
    uint32_t anisotropy = 16;
    DetailQuality materialQuality = DetailQuality::High;

    // Lighting / shadows
    ShadowQuality shadowQuality = ShadowQuality::High;
    float shadowDistance = 100.0f;
    bool contactShadows = true;
    AmbientOcclusionQuality aoQuality = AmbientOcclusionQuality::High;
    ReflectionQuality reflectionQuality = ReflectionQuality::High;
    DetailQuality globalIlluminationQuality = DetailQuality::Medium;

    // Lighting budgets
    uint32_t maxRenderedLights = kRendererMaxLights;
    uint32_t maxLocalShadowTiles = kRendererMaxLocalShadowTiles;

    // Shadow atlas and cascade overrides. Zero means quality-derived default
    // for atlas sizes and cascade count.
    uint32_t directionalShadowAtlasResolution = 0;
    uint32_t localShadowAtlasResolution = 0;
    uint32_t shadowCascadeCount = 0;

    // Directional cascade fitting
    float shadowCascadeSplitLambda = 0.95f;
    float shadowCascadeOverlapScale = 0.25f;
    float shadowCascadeMinOverlap = 4.0f;
    float shadowCascadeMaxOverlapScale = 0.10f;
    float shadowCascadeBlendScale = 0.12f;
    float shadowCascadeMinBlend = 0.75f;

    // Directional shadow filtering and bias. PCF radius 0 is a single
    // comparison sample; 1 is 3x3; 2 is 5x5.
    uint32_t directionalShadowPcfRadius = 1;
    float directionalShadowNormalOffsetScale = 0.75f;
    float directionalShadowNormalOffsetMin = 0.003f;
    float directionalShadowNormalOffsetMax = 0.03f;
    float directionalShadowBiasBase = 0.00045f;
    float directionalShadowBiasSlope = 0.0018f;
    float directionalShadowBiasMin = 0.00045f;
    float directionalShadowBiasMax = 0.003f;
    float directionalShadowDepthBiasConstant = 1.25f;
    float directionalShadowDepthBiasSlope = 1.75f;

    // Contact shadow tightening
    float contactShadowDistance = 35.0f;
    float contactShadowStrength = 0.75f;
    float contactShadowNormalOffsetScale = 0.20f;
    float contactShadowNormalOffsetMin = 0.0015f;
    float contactShadowBiasBase = 0.00012f;
    float contactShadowBiasSlope = 0.0008f;
    float contactShadowBiasMin = 0.00012f;
    float contactShadowBiasMax = 0.0012f;

    // Local point/spot shadow filtering and bias
    uint32_t localShadowPcfRadius = 1;
    float localShadowBiasScale = 0.0012f;
    float localShadowBiasMin = 0.0002f;
    float localShadowBiasMax = 0.004f;

    // Geometry
    DetailQuality modelQuality = DetailQuality::High;
    float lodDistanceMultiplier = 1.0f;
    DetailQuality crowdQuality = DetailQuality::Medium;
    DetailQuality arenaDetail = DetailQuality::High;
    DetailQuality boardGlassDetail = DetailQuality::High;

    // Effects
    bool bloom = true;
    bool motionBlur = false;
    bool depthOfField = false;
    bool lensFlare = true;
    DetailQuality volumetricLighting = DetailQuality::Medium;
    DetailQuality particleQuality = DetailQuality::High;

    // Hockey-specific quality
    DetailQuality iceQuality = DetailQuality::High;
    ReflectionQuality iceReflectionQuality = ReflectionQuality::High;
    DetailQuality iceScratchQuality = DetailQuality::High;
    EffectQuality skateSprayQuality = EffectQuality::High;
    EffectQuality puckTrailQuality = EffectQuality::Medium;
    DetailQuality jerseyQuality = DetailQuality::High;
    DetailQuality goalNetQuality = DetailQuality::High;

    // Post-processing
    AntiAliasing antiAliasing = AntiAliasing::FXAA;
    ToneMapper toneMapper = ToneMapper::ACES;
    bool filmGrain = false;
    bool chromaticAberration = false;
    bool vignette = false;

    // Debug / performance overlays
    bool showFPS = false;
    bool showFrameTime = false;
    bool showGPUStats = false;
    bool showNetworkStats = false;
};

// ---------------------------------------------------------------------------
// Construction / presets / persistence
// ---------------------------------------------------------------------------

RendererSettings MakeDefaultRendererSettings();
RendererSettings ApplyGraphicsPreset(GraphicsPreset preset, RendererSettings base = {});
RendererSettings ClampRendererSettings(RendererSettings settings);
uint32_t ResolveDirectionalShadowAtlasResolution(const RendererSettings& settings);
uint32_t ResolveLocalShadowAtlasResolution(const RendererSettings& settings);
uint32_t ResolveShadowCascadeCount(const RendererSettings& settings);
uint32_t ResolveDirectionalShadowPcfRadius(const RendererSettings& settings);
uint32_t ResolveLocalShadowPcfRadius(const RendererSettings& settings);

// Reads the [renderer] section from a Config into outSettings. Missing keys
// keep their default values. Always succeeds (returns the read settings).
Status LoadRendererSettings(const Config& config, RendererSettings& outSettings);
void SaveRendererSettings(Config& config, const RendererSettings& settings);

// ---------------------------------------------------------------------------
// Enum <-> string conversions (used for config + UI later)
// ---------------------------------------------------------------------------

const char* ToString(DisplayMode value);
const char* ToString(GraphicsPreset value);
const char* ToString(Upscaler value);
const char* ToString(AntiAliasing value);
const char* ToString(TextureQuality value);
const char* ToString(ShadowQuality value);
const char* ToString(AmbientOcclusionQuality value);
const char* ToString(ReflectionQuality value);
const char* ToString(EffectQuality value);
const char* ToString(DetailQuality value);
const char* ToString(ToneMapper value);

bool FromString(std::string_view text, DisplayMode& out);
bool FromString(std::string_view text, GraphicsPreset& out);
bool FromString(std::string_view text, Upscaler& out);
bool FromString(std::string_view text, AntiAliasing& out);
bool FromString(std::string_view text, TextureQuality& out);
bool FromString(std::string_view text, ShadowQuality& out);
bool FromString(std::string_view text, AmbientOcclusionQuality& out);
bool FromString(std::string_view text, ReflectionQuality& out);
bool FromString(std::string_view text, EffectQuality& out);
bool FromString(std::string_view text, DetailQuality& out);
bool FromString(std::string_view text, ToneMapper& out);

} // namespace Hockey
