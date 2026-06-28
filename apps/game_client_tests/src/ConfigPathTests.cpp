#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"

#include <filesystem>

namespace {

void TestConfiguredStartupScenesExist() {
    const std::filesystem::path root = Hockey::Paths::Get().root;

    Hockey::Config clientConfig;
    HK_CHECK(clientConfig.Load(root / "data/config/client.toml"));
    const std::filesystem::path clientScene = root / clientConfig.GetString("scene.startup_scene", "");
    HK_CHECK_MSG(std::filesystem::exists(clientScene), "client startup scene exists");

    Hockey::Config serverConfig;
    HK_CHECK(serverConfig.Load(root / "data/config/server.toml"));
    const std::filesystem::path serverScene = root / serverConfig.GetString("scene.startup_scene", "");
    HK_CHECK_MSG(std::filesystem::exists(serverScene), "server startup scene exists");
}

} // namespace

void RunConfigPathTests() {
    HockeyTest::BeginSuite("ConfigPath");
    TestConfiguredStartupScenesExist();
}
