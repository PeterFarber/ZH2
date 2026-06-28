#include "Test.hpp"

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Packaging/PackageCommand.hpp"
#include "Hockey/Editor/Packaging/PackageProfile.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

bool HasArg(const Hockey::PackageCommand& command, const std::string& arg) {
    return std::find(command.arguments.begin(), command.arguments.end(), arg) != command.arguments.end();
}

} // namespace

void RunPackageProfileTests() {
    HockeyTest::BeginSuite("PackageProfileTests");

    Hockey::PackageProfiles defaults = Hockey::MakeDefaultPackageProfiles();
    HK_CHECK(defaults.client.enabled);
    HK_CHECK(defaults.server.enabled);
    HK_CHECK_EQ(defaults.client.preset, std::string("windows-release"));
    HK_CHECK_EQ(defaults.server.buildMode, std::string("Release"));
    HK_CHECK_EQ(defaults.client.outputDir.generic_string(), std::string("out/packages/client"));
    HK_CHECK_EQ(defaults.server.outputDir.generic_string(), std::string("out/packages/server"));

    const std::filesystem::path workspace = Hockey::Paths::TempFile("package_profile_tests");
    Hockey::FileSystem::Remove(workspace);
    const std::filesystem::path profilePath = workspace / "data" / "editor" / "package_profiles.toml";
    defaults.client.preset = "windows-release";
    defaults.client.outputDir = "out/packages/client-custom";
    defaults.server.includeDebugSymbols = true;
    HK_CHECK(Hockey::SavePackageProfiles(defaults, profilePath));
    const Hockey::Result<Hockey::PackageProfiles> loaded = Hockey::LoadPackageProfiles(profilePath);
    HK_CHECK(loaded);
    HK_CHECK_EQ(loaded ? loaded.value.client.outputDir.generic_string() : std::string{},
                std::string("out/packages/client-custom"));
    HK_CHECK(loaded && loaded.value.server.includeDebugSymbols);

    const Hockey::Result<std::filesystem::path> resolved =
        Hockey::ResolvePackageOutputDir(workspace, "out/packages/client");
    HK_CHECK(resolved);
    HK_CHECK_EQ(resolved ? resolved.value.generic_string() : std::string{},
                (workspace / "out/packages/client").lexically_normal().generic_string());
    HK_CHECK(!Hockey::ResolvePackageOutputDir(workspace, "../outside"));

    const Hockey::PackageCommand clientCommand = Hockey::MakeClientPackageCommand(defaults.client, workspace);
#if HK_PLATFORM_WINDOWS
    HK_CHECK(clientCommand.executable.generic_string().find("scripts/windows/package_client.ps1") !=
             std::string::npos);
    HK_CHECK(HasArg(clientCommand, "-Preset"));
    HK_CHECK(HasArg(clientCommand, "-OutputDir"));
#else
    HK_CHECK(clientCommand.executable.generic_string().find("scripts/linux/package_client.sh") != std::string::npos);
    HK_CHECK(HasArg(clientCommand, "--preset"));
    HK_CHECK(HasArg(clientCommand, "--output-dir"));
#endif
    HK_CHECK(HasArg(clientCommand, "windows-release"));
    HK_CHECK(HasArg(clientCommand, "out/packages/client-custom"));
    HK_CHECK_EQ(clientCommand.workingDirectory, workspace);

    Hockey::FileSystem::Remove(workspace);
}
