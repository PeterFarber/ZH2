#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Renderer/RendererSettings.hpp"

using namespace Hockey;

void RunRendererSettingsTests() {
    HockeyTest::BeginSuite("RendererSettingsTests");

    // Default settings are valid / sensible.
    const RendererSettings def = MakeDefaultRendererSettings();
    HK_CHECK(def.resolutionWidth == 1920);
    HK_CHECK(def.resolutionHeight == 1080);
    HK_CHECK(def.preset == GraphicsPreset::High);
    HK_CHECK(def.toneMapper == ToneMapper::ACES);
    HK_CHECK(def.antiAliasing == AntiAliasing::FXAA);

    // Presets change expected fields.
    const RendererSettings low = ApplyGraphicsPreset(GraphicsPreset::Low);
    HK_CHECK(low.preset == GraphicsPreset::Low);
    HK_CHECK(low.shadowQuality == ShadowQuality::Low);
    HK_CHECK(low.aoQuality == AmbientOcclusionQuality::Off);
    HK_CHECK(low.bloom == false);

    const RendererSettings medium = ApplyGraphicsPreset(GraphicsPreset::Medium);
    HK_CHECK(medium.preset == GraphicsPreset::Medium);
    HK_CHECK(medium.shadowQuality == ShadowQuality::Medium);
    HK_CHECK(medium.textureQuality == TextureQuality::Medium);

    const RendererSettings high = ApplyGraphicsPreset(GraphicsPreset::High);
    HK_CHECK(high.shadowQuality == ShadowQuality::High);
    HK_CHECK(high.anisotropy == 16);

    const RendererSettings ultra = ApplyGraphicsPreset(GraphicsPreset::Ultra);
    HK_CHECK(ultra.shadowQuality == ShadowQuality::Ultra);
    HK_CHECK(ultra.textureQuality == TextureQuality::Ultra);
    HK_CHECK(ultra.antiAliasing == AntiAliasing::TAA);

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
}
