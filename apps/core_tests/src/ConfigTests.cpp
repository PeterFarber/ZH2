#include "Test.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
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

    FileSystem::Remove(tempPath);
    FileSystem::Remove(badPath);
}
