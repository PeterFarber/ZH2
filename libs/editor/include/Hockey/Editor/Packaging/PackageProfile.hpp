#pragma once

#include "Hockey/Core/Result.hpp"

#include <filesystem>
#include <string>

namespace Hockey {

struct PackageProfile {
    bool enabled = true;
    std::string preset = "windows-release";
    std::string buildMode = "Release";
    std::filesystem::path outputDir;
    bool includeDebugSymbols = false;
};

struct PackageProfiles {
    PackageProfile client;
    PackageProfile server;
};

PackageProfiles MakeDefaultPackageProfiles();
std::filesystem::path DefaultPackageProfilesPath();

Result<PackageProfiles> LoadPackageProfiles(const std::filesystem::path& path = DefaultPackageProfilesPath());
Status SavePackageProfiles(const PackageProfiles& profiles,
                           const std::filesystem::path& path = DefaultPackageProfilesPath());
Result<std::filesystem::path> ResolvePackageOutputDir(const std::filesystem::path& projectRoot,
                                                      const std::filesystem::path& outputDir);

} // namespace Hockey
