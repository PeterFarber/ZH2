#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"

using namespace Hockey;

namespace Hockey {
// Declared in Vulkan/VulkanFrameTargets.hpp; forward-declared here so the test
// avoids pulling in the Vulkan/volk headers just to exercise the pure mapping.
uint32_t ShadowAtlasResolution(ShadowQuality quality);
uint32_t ShadowCascadeCount(ShadowQuality quality);
} // namespace Hockey

void RunRendererSettingsTests() {
    HockeyTest::BeginSuite("RendererSettingsTests");

    // Default settings are valid / sensible.
    const RendererSettings def = MakeDefaultRendererSettings();
    HK_CHECK(def.resolutionWidth == 1920);
    HK_CHECK(def.resolutionHeight == 1080);
    HK_CHECK(def.preset == GraphicsPreset::High);
    HK_CHECK(def.toneMapper == ToneMapper::ACES);
    HK_CHECK(def.antiAliasing == AntiAliasing::FXAA);
    HK_CHECK(def.decals == true);
    HK_CHECK_EQ(def.maxRenderedDecals, kRendererMaxDecals);

    // Presets change expected fields.
    const RendererSettings low = ApplyGraphicsPreset(GraphicsPreset::Low);
    HK_CHECK(low.preset == GraphicsPreset::Low);
    HK_CHECK(low.shadowQuality == ShadowQuality::Low);
    HK_CHECK(low.aoQuality == AmbientOcclusionQuality::Off);
    HK_CHECK(low.bloom == false);
    HK_CHECK(low.decals == true);
    HK_CHECK_EQ(low.maxRenderedDecals, 8u);
    HK_CHECK_EQ(low.directionalShadowAtlasResolution, 1024u);
    HK_CHECK_EQ(low.localShadowAtlasResolution, 1024u);
    HK_CHECK_EQ(low.shadowCascadeCount, 2u);

    const RendererSettings medium = ApplyGraphicsPreset(GraphicsPreset::Medium);
    HK_CHECK(medium.preset == GraphicsPreset::Medium);
    HK_CHECK(medium.shadowQuality == ShadowQuality::Medium);
    HK_CHECK(medium.textureQuality == TextureQuality::Medium);
    HK_CHECK(medium.decals == true);
    HK_CHECK_EQ(medium.maxRenderedDecals, 16u);
    HK_CHECK_EQ(medium.directionalShadowAtlasResolution, 2048u);
    HK_CHECK_EQ(medium.localShadowAtlasResolution, 2048u);
    HK_CHECK_EQ(medium.shadowCascadeCount, 3u);

    const RendererSettings high = ApplyGraphicsPreset(GraphicsPreset::High);
    HK_CHECK(high.shadowQuality == ShadowQuality::High);
    HK_CHECK(high.anisotropy == 16);
    HK_CHECK(high.decals == true);
    HK_CHECK_EQ(high.maxRenderedDecals, kRendererMaxDecals);
    HK_CHECK_EQ(high.directionalShadowAtlasResolution, 4096u);
    HK_CHECK_EQ(high.localShadowAtlasResolution, 2048u);
    HK_CHECK_EQ(high.shadowCascadeCount, 4u);

    const RendererSettings ultra = ApplyGraphicsPreset(GraphicsPreset::Ultra);
    HK_CHECK(ultra.shadowQuality == ShadowQuality::Ultra);
    HK_CHECK(ultra.textureQuality == TextureQuality::Ultra);
    HK_CHECK(ultra.antiAliasing == AntiAliasing::TAA);
    HK_CHECK(ultra.decals == true);
    HK_CHECK_EQ(ultra.maxRenderedDecals, kRendererMaxDecals);
    HK_CHECK_EQ(ultra.directionalShadowAtlasResolution, 8192u);
    HK_CHECK_EQ(ultra.localShadowAtlasResolution, 4096u);
    HK_CHECK_EQ(ultra.shadowCascadeCount, 4u);

    // Enum <-> string round-trips.
    HK_CHECK(std::string(ToString(ToneMapper::ACES)) == "ACES");
    HK_CHECK(std::string(ToString(ShadowQuality::Ultra)) == "Ultra");
    HK_CHECK(std::string(ToString(DisplayMode::BorderlessFullscreen)) == "BorderlessFullscreen");

    ToneMapper tm{};
    HK_CHECK(FromString("Reinhard", tm) && tm == ToneMapper::Reinhard);
    HK_CHECK(FromString("aces", tm) && tm == ToneMapper::ACES); // case-insensitive
    ShadowQuality sq{};
    HK_CHECK(FromString("High", sq) && sq == ShadowQuality::High);
    AntiAliasing aa{};
    HK_CHECK(!FromString("NotARealValue", aa));

    // Save to config then load back gives identical values for representative fields.
    Config config;
    RendererSettings saved = ApplyGraphicsPreset(GraphicsPreset::Ultra);
    saved.vsync = false;
    saved.fieldOfView = 95.0f;
    saved.anisotropy = 8;
    saved.bloom = false;
    SaveRendererSettings(config, saved);

    RendererSettings loaded = MakeDefaultRendererSettings();
    const Status status = LoadRendererSettings(config, loaded);
    HK_CHECK(static_cast<bool>(status));
    HK_CHECK(loaded.preset == GraphicsPreset::Ultra);
    HK_CHECK(loaded.vsync == false);
    HK_CHECK_NEAR(loaded.fieldOfView, 95.0f, 0.001);
    HK_CHECK(loaded.anisotropy == 8);
    HK_CHECK(loaded.bloom == false);
    HK_CHECK(loaded.shadowQuality == ShadowQuality::Ultra);
    HK_CHECK(loaded.toneMapper == saved.toneMapper);

    // Advanced lighting/shadow settings round-trip and clamp through TOML.
    Config advancedConfig;
    RendererSettings advanced = MakeDefaultRendererSettings();
    advanced.maxRenderedLights = 12;
    advanced.maxLocalShadowTiles = 10;
    advanced.directionalShadowAtlasResolution = 6144;
    advanced.localShadowAtlasResolution = 3072;
    advanced.decals = false;
    advanced.maxRenderedDecals = 12;
    advanced.shadowCascadeCount = 3;
    advanced.shadowCascadeSplitLambda = 0.92f;
    advanced.shadowCascadeOverlapScale = 0.30f;
    advanced.shadowCascadeMinOverlap = 3.0f;
    advanced.shadowCascadeMaxOverlapScale = 0.12f;
    advanced.shadowCascadeBlendScale = 0.18f;
    advanced.shadowCascadeMinBlend = 1.25f;
    advanced.directionalShadowPcfRadius = 2;
    advanced.directionalShadowNormalOffsetScale = 0.65f;
    advanced.directionalShadowNormalOffsetMin = 0.002f;
    advanced.directionalShadowNormalOffsetMax = 0.05f;
    advanced.directionalShadowBiasBase = 0.0005f;
    advanced.directionalShadowBiasSlope = 0.0016f;
    advanced.directionalShadowBiasMin = 0.0003f;
    advanced.directionalShadowBiasMax = 0.004f;
    advanced.directionalShadowDepthBiasConstant = 1.5f;
    advanced.directionalShadowDepthBiasSlope = 2.0f;
    advanced.contactShadowDistance = 42.0f;
    advanced.contactShadowStrength = 0.65f;
    advanced.contactShadowNormalOffsetScale = 0.25f;
    advanced.contactShadowNormalOffsetMin = 0.001f;
    advanced.contactShadowBiasBase = 0.0001f;
    advanced.contactShadowBiasSlope = 0.0007f;
    advanced.contactShadowBiasMin = 0.00008f;
    advanced.contactShadowBiasMax = 0.001f;
    advanced.localShadowPcfRadius = 2;
    advanced.localShadowBiasScale = 0.0009f;
    advanced.localShadowBiasMin = 0.00015f;
    advanced.localShadowBiasMax = 0.003f;
    SaveRendererSettings(advancedConfig, advanced);

    RendererSettings advancedLoaded = MakeDefaultRendererSettings();
    HK_CHECK(static_cast<bool>(LoadRendererSettings(advancedConfig, advancedLoaded)));
    HK_CHECK_EQ(advancedLoaded.maxRenderedLights, 12u);
    HK_CHECK_EQ(advancedLoaded.maxLocalShadowTiles, 10u);
    HK_CHECK_EQ(advancedLoaded.directionalShadowAtlasResolution, 6144u);
    HK_CHECK_EQ(advancedLoaded.localShadowAtlasResolution, 3072u);
    HK_CHECK(advancedLoaded.decals == false);
    HK_CHECK_EQ(advancedLoaded.maxRenderedDecals, 12u);
    HK_CHECK_EQ(advancedLoaded.shadowCascadeCount, 3u);
    HK_CHECK_NEAR(advancedLoaded.shadowCascadeSplitLambda, 0.92f, 0.0001f);
    HK_CHECK_NEAR(advancedLoaded.shadowCascadeOverlapScale, 0.30f, 0.0001f);
    HK_CHECK_NEAR(advancedLoaded.shadowCascadeBlendScale, 0.18f, 0.0001f);
    HK_CHECK_EQ(advancedLoaded.directionalShadowPcfRadius, 2u);
    HK_CHECK_NEAR(advancedLoaded.directionalShadowNormalOffsetScale, 0.65f, 0.0001f);
    HK_CHECK_NEAR(advancedLoaded.directionalShadowBiasSlope, 0.0016f, 0.00001f);
    HK_CHECK_NEAR(advancedLoaded.directionalShadowDepthBiasSlope, 2.0f, 0.0001f);
    HK_CHECK_NEAR(advancedLoaded.contactShadowDistance, 42.0f, 0.001f);
    HK_CHECK_NEAR(advancedLoaded.contactShadowStrength, 0.65f, 0.0001f);
    HK_CHECK_EQ(advancedLoaded.localShadowPcfRadius, 2u);
    HK_CHECK_NEAR(advancedLoaded.localShadowBiasMax, 0.003f, 0.00001f);

    Config unsafeConfig;
    unsafeConfig.SetInt("renderer.max_rendered_lights", 1000);
    unsafeConfig.SetInt("renderer.max_local_shadow_tiles", 1000);
    unsafeConfig.SetInt("renderer.max_rendered_decals", 9999);
    unsafeConfig.SetInt("renderer.directional_shadow_atlas_resolution", -4);
    unsafeConfig.SetInt("renderer.local_shadow_atlas_resolution", -8);
    unsafeConfig.SetInt("renderer.shadow_cascade_count", 20);
    unsafeConfig.SetDouble("renderer.shadow_cascade_split_lambda", 5.0);
    unsafeConfig.SetDouble("renderer.contact_shadow_strength", 9.0);
    unsafeConfig.SetInt("renderer.directional_shadow_pcf_radius", 99);
    RendererSettings clamped = MakeDefaultRendererSettings();
    HK_CHECK(static_cast<bool>(LoadRendererSettings(unsafeConfig, clamped)));
    HK_CHECK_EQ(clamped.maxRenderedLights, 16u);
    HK_CHECK_EQ(clamped.maxLocalShadowTiles, 16u);
    HK_CHECK_EQ(clamped.maxRenderedDecals, kRendererMaxDecals);
    HK_CHECK_EQ(clamped.directionalShadowAtlasResolution, 1u);
    HK_CHECK_EQ(clamped.localShadowAtlasResolution, 1u);
    HK_CHECK_EQ(clamped.shadowCascadeCount, 4u);
    HK_CHECK_NEAR(clamped.shadowCascadeSplitLambda, 1.0f, 0.0001f);
    HK_CHECK_NEAR(clamped.contactShadowStrength, 1.0f, 0.0001f);
    HK_CHECK_EQ(clamped.directionalShadowPcfRadius, 3u);

    Config legacyPresetSentinelConfig;
    legacyPresetSentinelConfig.SetInt("renderer.directional_shadow_atlas_resolution", 0);
    legacyPresetSentinelConfig.SetInt("renderer.local_shadow_atlas_resolution", 0);
    legacyPresetSentinelConfig.SetInt("renderer.shadow_cascade_count", 0);
    RendererSettings migrated = MakeDefaultRendererSettings();
    HK_CHECK(static_cast<bool>(LoadRendererSettings(legacyPresetSentinelConfig, migrated)));
    HK_CHECK_EQ(migrated.directionalShadowAtlasResolution, 4096u);
    HK_CHECK_EQ(migrated.localShadowAtlasResolution, 2048u);
    HK_CHECK_EQ(migrated.shadowCascadeCount, 4u);
}

void RunShadowQualityTests() {
    HockeyTest::BeginSuite("ShadowQualityTests");

    // Atlas resolution scales monotonically with quality; Off is a 1x1 stub.
    HK_CHECK_EQ(ShadowAtlasResolution(ShadowQuality::Off), 1u);
    HK_CHECK_EQ(ShadowAtlasResolution(ShadowQuality::Low), 1024u);
    HK_CHECK_EQ(ShadowAtlasResolution(ShadowQuality::Medium), 2048u);
    HK_CHECK_EQ(ShadowAtlasResolution(ShadowQuality::High), 4096u);
    HK_CHECK_EQ(ShadowAtlasResolution(ShadowQuality::Ultra), 8192u);
    HK_CHECK(ShadowAtlasResolution(ShadowQuality::Ultra) > ShadowAtlasResolution(ShadowQuality::High));

    // Cascade count: Off disables shadows (0), increasing with quality.
    HK_CHECK_EQ(ShadowCascadeCount(ShadowQuality::Off), 0u);
    HK_CHECK_EQ(ShadowCascadeCount(ShadowQuality::Low), 2u);
    HK_CHECK_EQ(ShadowCascadeCount(ShadowQuality::Medium), 3u);
    HK_CHECK_EQ(ShadowCascadeCount(ShadowQuality::High), 4u);
    HK_CHECK(ShadowCascadeCount(ShadowQuality::Ultra) >= ShadowCascadeCount(ShadowQuality::High));

    // Higher quality never reduces resolution or cascade count.
    HK_CHECK(ShadowAtlasResolution(ShadowQuality::Medium) > ShadowAtlasResolution(ShadowQuality::Low));
    HK_CHECK(ShadowCascadeCount(ShadowQuality::High) > ShadowCascadeCount(ShadowQuality::Low));

    RendererSettings concreteDefaults = MakeDefaultRendererSettings();
    concreteDefaults.shadowQuality = ShadowQuality::High;
    HK_CHECK_EQ(ResolveDirectionalShadowAtlasResolution(concreteDefaults), 4096u);
    HK_CHECK_EQ(ResolveLocalShadowAtlasResolution(concreteDefaults), 2048u);
    HK_CHECK_EQ(ResolveShadowCascadeCount(concreteDefaults), 4u);
    HK_CHECK_EQ(ResolveDirectionalShadowPcfRadius(concreteDefaults), 1u);
    HK_CHECK_EQ(ResolveLocalShadowPcfRadius(concreteDefaults), 1u);

    RendererSettings explicitShadows = concreteDefaults;
    explicitShadows.directionalShadowAtlasResolution = 6144;
    explicitShadows.localShadowAtlasResolution = 3072;
    explicitShadows.shadowCascadeCount = 2;
    explicitShadows.directionalShadowPcfRadius = 0;
    explicitShadows.localShadowPcfRadius = 0;
    HK_CHECK_EQ(ResolveDirectionalShadowAtlasResolution(explicitShadows), 6144u);
    HK_CHECK_EQ(ResolveLocalShadowAtlasResolution(explicitShadows), 3072u);
    HK_CHECK_EQ(ResolveShadowCascadeCount(explicitShadows), 2u);
    HK_CHECK_EQ(ResolveDirectionalShadowPcfRadius(explicitShadows), 0u);
    HK_CHECK_EQ(ResolveLocalShadowPcfRadius(explicitShadows), 0u);
}
