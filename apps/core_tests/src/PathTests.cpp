#include "Test.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
using namespace Hockey;
void RunPathTests() {
    HockeyTest::BeginSuite("PathTests");

    const EnginePaths& paths = Paths::Get();
    HK_CHECK(!paths.root.empty());
    HK_CHECK(paths.root.is_absolute());

    HK_CHECK(!Paths::ConfigFile("client.toml").empty());
    HK_CHECK(!Paths::LogFile("core_tests.log").empty());
    HK_CHECK(!Paths::TempFile("scratch.tmp").empty());

    HK_CHECK_EQ(Paths::ConfigFile("client.toml"), paths.config / "client.toml");
    HK_CHECK_EQ(Paths::LogFile("core_tests.log"), paths.logs / "core_tests.log");

    HK_CHECK_EQ(Paths::DataFile("config/client.toml"), paths.data / "config/client.toml");
    HK_CHECK_EQ(Paths::RawAsset("models/rink.glb"), paths.rawAssets / "models/rink.glb");
    HK_CHECK_EQ(Paths::CookedAsset("models/rink.mesh.bin"), paths.cookedAssets / "models/rink.mesh.bin");

    HK_CHECK(FileSystem::IsDirectory(paths.logs));
    HK_CHECK(FileSystem::IsDirectory(paths.temp));

    // FileSystem::Normalize collapses '.'/'..' segments; two equivalent paths
    // must normalize to the same canonical form.
    HK_CHECK_EQ(FileSystem::Normalize(paths.data / "config" / ".."),
                FileSystem::Normalize(paths.data));

    // Paths::Resolve: relative paths anchor at root, absolute paths pass through.
    HK_CHECK_EQ(Paths::Resolve("data/config"), (paths.root / "data/config").lexically_normal());
    HK_CHECK_EQ(Paths::Resolve(paths.logs / "core_tests.log"),
                (paths.logs / "core_tests.log").lexically_normal());
    HK_CHECK(Paths::Resolve("relative/thing").is_absolute());
}
