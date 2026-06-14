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

    HK_CHECK(FileSystem::IsDirectory(paths.logs));
    HK_CHECK(FileSystem::IsDirectory(paths.temp));
}
