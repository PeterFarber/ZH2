#include "Test.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/RuntimeConfig.hpp"
#include "Hockey/Core/RuntimeConfigDefaults.hpp"
#include <filesystem>
#include <string>
using namespace Hockey;
void RunConfigTests() {
    HockeyTest::BeginSuite("ConfigTests");

    Config editor;
    HK_CHECK_MSG(static_cast<bool>(editor.Load(Paths::ConfigFile("editor.toml"))),
                 "loads editor.toml");
    HK_CHECK_EQ(editor.GetString("app.name", ""), std::string("Hockey"));
    HK_CHECK_EQ(editor.GetInt("window.width", -1), 1600);
    HK_CHECK_EQ(editor.GetInt("window.height", -1), 900);
    HK_CHECK_EQ(editor.GetInt("renderer.directional_shadow_atlas_resolution", 0), 8192);
    HK_CHECK_EQ(editor.GetInt("renderer.local_shadow_atlas_resolution", 0), 4096);
    HK_CHECK_EQ(editor.GetInt("renderer.shadow_cascade_count", 0), 4);
    HK_CHECK_EQ(editor.GetInt("does.not.exist", 42), 42);
    HK_CHECK(!FileSystem::Exists(Paths::ConfigFile("client.toml")));
    HK_CHECK(!FileSystem::Exists(Paths::ConfigFile("server.toml")));

    Config builtInDefaults;
    HK_CHECK_MSG(builtInDefaults.LoadString(std::string(DefaultRuntimeConfigToml()), "built-in-runtime-defaults"),
                 "built-in runtime defaults parse as TOML");
    HK_CHECK_EQ(builtInDefaults.GetString("app.name", ""), std::string("Hockey"));
    HK_CHECK_EQ(builtInDefaults.GetString("window.title", ""), std::string("Hockey"));
    HK_CHECK_EQ(builtInDefaults.GetInt("app.target_fps", 0), 144);
    HK_CHECK_EQ(builtInDefaults.GetInt("window.width", -1), 1600);
    HK_CHECK(builtInDefaults.GetBool("input.gamepad_enabled", false));
    HK_CHECK(builtInDefaults.GetBool("audio.enabled", false));
    HK_CHECK_EQ(builtInDefaults.GetString("audio.backend", ""), std::string("Auto"));
    HK_CHECK_EQ(builtInDefaults.GetDouble("audio.master_volume", 0.0), 1.0);
    HK_CHECK(builtInDefaults.Has("renderer.vsync"));
    HK_CHECK(builtInDefaults.Has("physics.fixed_delta_seconds"));
    HK_CHECK(builtInDefaults.Has("gameplay.local_play"));
    HK_CHECK(builtInDefaults.Has("gameplay.authoritative"));
    HK_CHECK(builtInDefaults.Has("scene.startup_scene"));
    HK_CHECK(builtInDefaults.Has("server.tick_rate"));
    HK_CHECK(builtInDefaults.Has("ui.start_flow"));
    HK_CHECK(builtInDefaults.Has("camera.follow_local_player"));
    HK_CHECK(builtInDefaults.Has("presentation.interpolate_gameplay"));
    HK_CHECK_EQ(builtInDefaults.GetString("renderer.preset", ""), std::string("High"));
    HK_CHECK_EQ(builtInDefaults.GetString("renderer.shadow_quality", ""), std::string("High"));
    HK_CHECK_EQ(builtInDefaults.GetString("renderer.reflection_quality", ""), std::string("High"));
    HK_CHECK_EQ(builtInDefaults.GetInt("renderer.directional_shadow_atlas_resolution", 0), 4096);
    HK_CHECK_EQ(builtInDefaults.GetInt("renderer.local_shadow_atlas_resolution", 0), 2048);
    HK_CHECK_EQ(builtInDefaults.GetInt("renderer.shadow_cascade_count", 0), 4);
    HK_CHECK(builtInDefaults.GetBool("renderer.hdr", false));
    HK_CHECK(builtInDefaults.GetBool("renderer.bloom", false));
    HK_CHECK(builtInDefaults.GetBool("renderer.contact_shadows", false));
    HK_CHECK(builtInDefaults.GetBool("renderer.decals", false));
    HK_CHECK_EQ(builtInDefaults.GetDouble("renderer.field_of_view", 0.0), 70.0);
    HK_CHECK_EQ(builtInDefaults.GetDouble("renderer.shadow_distance", 0.0), 100.0);
    HK_CHECK_EQ(builtInDefaults.GetDouble("renderer.sharpening", 0.0), 0.3);
    HK_CHECK(builtInDefaults.GetBool("renderer.lens_flare", false));
    HK_CHECK(!builtInDefaults.GetBool("renderer.show_fps", true));
    HK_CHECK(!builtInDefaults.GetBool("renderer.show_frame_time", true));
    HK_CHECK(!builtInDefaults.GetBool("renderer.show_gpu_stats", true));
    HK_CHECK(!builtInDefaults.GetBool("renderer.show_network_stats", true));
    HK_CHECK(!builtInDefaults.GetBool("physics.enable_debug_draw", true));
    HK_CHECK(!builtInDefaults.GetBool("gameplay.debug_draw_gameplay", true));
    HK_CHECK(!builtInDefaults.GetBool("gameplay.log_gameplay_events", true));
    HK_CHECK_EQ(builtInDefaults.GetDouble("gameplay.pregame_countdown_seconds", 0.0), 3.0);

    Config madeDefaults = MakeDefaultRuntimeConfig();
    HK_CHECK_EQ(madeDefaults.GetString("scene.startup_scene", ""), builtInDefaults.GetString("scene.startup_scene", ""));
    HK_CHECK_EQ(madeDefaults.GetBool("renderer.vsync", false), builtInDefaults.GetBool("renderer.vsync", false));
    HK_CHECK_EQ(madeDefaults.GetInt("renderer.directional_shadow_atlas_resolution", 0), 4096);
    HK_CHECK_EQ(madeDefaults.GetInt("renderer.local_shadow_atlas_resolution", 0), 2048);
    HK_CHECK_EQ(madeDefaults.GetInt("renderer.shadow_cascade_count", 0), 4);

    Config out;
    out.SetString("app.name", "Temp");
    out.SetInt("server.port", 27020);
    out.SetBool("flags.enabled", true);
    out.SetDouble("physics.gravity", 9.81);
    const auto tempPath = Paths::TempFile("test_config.toml");
    HK_CHECK_MSG(static_cast<bool>(out.Save(tempPath)), "saves temp config");

    Config reloaded;
    HK_CHECK_MSG(static_cast<bool>(reloaded.Load(tempPath)), "reloads temp config");
    HK_CHECK_EQ(reloaded.GetString("app.name", ""), std::string("Temp"));
    HK_CHECK_EQ(reloaded.GetInt("server.port", 0), 27020);
    HK_CHECK(reloaded.GetBool("flags.enabled", false));
    HK_CHECK(reloaded.GetDouble("physics.gravity", 0.0) > 9.8);
    HK_CHECK(reloaded.Has("app.name"));
    HK_CHECK(!reloaded.Has("missing.key"));

    Config bad;
    const auto badPath = Paths::TempFile("bad_config.toml");
    FileSystem::WriteTextFile(badPath, "this is = = not valid toml ][");
    HK_CHECK_MSG(!bad.Load(badPath), "bad config returns failure");

    Config defaults;
    HK_CHECK(defaults.LoadString("[window]\nwidth = 1920\nheight = 1080\n[renderer]\nvsync = true\n",
                                 "embedded-client-defaults"));
    Config override;
    HK_CHECK(override.LoadString("[window]\nwidth = 1280\n", "user-client-config"));
    defaults.MergeFrom(override);
    HK_CHECK_EQ(defaults.GetInt("window.width", 0), 1280);
    HK_CHECK_EQ(defaults.GetInt("window.height", 0), 1080);
    HK_CHECK(defaults.GetBool("renderer.vsync", false));
    const std::string serialized = defaults.ToTomlString();
    HK_CHECK(serialized.find("width = 1280") != std::string::npos);
    HK_CHECK(serialized.find("height = 1080") != std::string::npos);

    const std::filesystem::path originalRoot = Paths::Get().root;
    const std::filesystem::path fakeRuntimeRoot = Paths::TempFile("runtime_config_root");
    const std::filesystem::path fakeExe = fakeRuntimeRoot / "HockeyGameClient.exe";
    std::filesystem::remove_all(fakeRuntimeRoot);
    std::filesystem::create_directories(fakeRuntimeRoot);
    FileSystem::WriteTextFile(fakeRuntimeRoot / "HockeyGameClient.toml", "[window]\nwidth = 1600\n");
    HK_CHECK(Paths::Init(fakeExe, {}));

    const RuntimeConfigLoadInfo info{
        .embeddedToml = DefaultRuntimeConfigToml(),
        .embeddedSourceName = "built-in-runtime-defaults",
        .siblingFilename = "HockeyGameClient.toml",
    };
    const Result<RuntimeConfigLoadResult> loaded = LoadRuntimeConfig(info);
    HK_CHECK(loaded);
    HK_CHECK_EQ(loaded.value.config.GetInt("window.width", 0), 1600);
    HK_CHECK_EQ(loaded.value.config.GetInt("window.height", 0), builtInDefaults.GetInt("window.height", 0));
    HK_CHECK_EQ(loaded.value.config.GetString("scene.startup_scene", ""),
                builtInDefaults.GetString("scene.startup_scene", ""));
    HK_CHECK_EQ(loaded.value.userConfigPath, fakeRuntimeRoot / "HockeyGameClient.toml");
    HK_CHECK(loaded.value.loadedUserOverride);
    HK_CHECK(!loaded.value.usedCommandLineOverride);
    HK_CHECK(Paths::Init(originalRoot / "build" / "windows-debug" / "apps" / "core_tests" / "HockeyCoreTests.exe",
                         originalRoot));
    std::filesystem::remove_all(fakeRuntimeRoot);

    FileSystem::Remove(tempPath);
    FileSystem::Remove(badPath);
}
