#include "Test.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/RuntimeConfig.hpp"
#include <filesystem>
#include <string>
using namespace Hockey;
void RunConfigTests() {
    HockeyTest::BeginSuite("ConfigTests");

    Config client;
    HK_CHECK_MSG(static_cast<bool>(client.Load(Paths::ConfigFile("client.toml"))),
                 "loads client.toml");
    HK_CHECK_EQ(client.GetInt("window.width", -1), 1920);
    HK_CHECK_EQ(client.GetInt("window.height", -1), 1080);
    HK_CHECK_EQ(client.GetInt("does.not.exist", 42), 42);

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
        .embeddedToml = "[window]\nwidth = 1920\nheight = 1080\n",
        .embeddedSourceName = "embedded-client-defaults",
        .siblingFilename = "HockeyGameClient.toml",
    };
    const Result<RuntimeConfigLoadResult> loaded = LoadRuntimeConfig(info);
    HK_CHECK(loaded);
    HK_CHECK_EQ(loaded.value.config.GetInt("window.width", 0), 1600);
    HK_CHECK_EQ(loaded.value.config.GetInt("window.height", 0), 1080);
    HK_CHECK_EQ(loaded.value.userConfigPath, fakeRuntimeRoot / "HockeyGameClient.toml");
    HK_CHECK(loaded.value.loadedUserOverride);
    HK_CHECK(!loaded.value.usedCommandLineOverride);
    HK_CHECK(Paths::Init(originalRoot / "build" / "windows-debug" / "apps" / "core_tests" / "HockeyCoreTests.exe",
                         originalRoot));
    std::filesystem::remove_all(fakeRuntimeRoot);

    FileSystem::Remove(tempPath);
    FileSystem::Remove(badPath);
}
