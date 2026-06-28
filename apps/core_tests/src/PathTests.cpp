#include "Test.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include <filesystem>
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

    // Root inference prefers a build-local data directory when no --root
    // override is supplied.
    const std::filesystem::path originalRoot = paths.root;
    const std::filesystem::path fakeBuildRoot = Paths::TempFile("path_inference_build_root");
    const std::filesystem::path fakeExe = fakeBuildRoot / "apps" / "game_client" / "HockeyGameClient.exe";
    std::filesystem::remove_all(fakeBuildRoot);
    std::filesystem::create_directories(fakeBuildRoot / "data" / "config");
    std::filesystem::create_directories(fakeExe.parent_path());
    HK_CHECK(Paths::Init(fakeExe, {}));
    HK_CHECK_EQ(Paths::Get().root, fakeBuildRoot.lexically_normal());
    HK_CHECK_EQ(Paths::ExecutableDirectory(), fakeExe.parent_path());
    HK_CHECK_EQ(Paths::ExecutableSiblingFile("HockeyGameClient.toml"),
                fakeExe.parent_path() / "HockeyGameClient.toml");
    HK_CHECK_EQ(Paths::ConfigFile("client.toml"), fakeBuildRoot / "data" / "config" / "client.toml");
    HK_CHECK(Paths::Init(originalRoot / "build" / "windows-debug" / "apps" / "core_tests" / "HockeyCoreTests.exe",
                         originalRoot));
}
