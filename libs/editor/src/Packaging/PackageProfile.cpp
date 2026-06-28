#include "Hockey/Editor/Packaging/PackageProfile.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/ResourceProvider.hpp"

namespace Hockey {

PackageProfiles MakeDefaultPackageProfiles() {
    PackageProfiles profiles;
    profiles.client.outputDir = "out/packages/client";
    profiles.server.outputDir = "out/packages/server";
    return profiles;
}

std::filesystem::path DefaultPackageProfilesPath() {
    return Paths::DataFile("editor/package_profiles.toml");
}

Result<PackageProfiles> LoadPackageProfiles(const std::filesystem::path& path) {
    PackageProfiles profiles = MakeDefaultPackageProfiles();
    if (!FileSystem::Exists(path)) {
        return Result<PackageProfiles>::Ok(std::move(profiles));
    }

    Config config;
    if (const Status loaded = config.Load(path); !loaded) {
        return Result<PackageProfiles>::Fail(loaded.error);
    }

    auto loadOne = [&config](const char* prefix, PackageProfile& profile) {
        const std::string base(prefix);
        profile.enabled = config.GetBool(base + ".enabled", profile.enabled);
        profile.preset = config.GetString(base + ".preset", profile.preset);
        profile.buildMode = config.GetString(base + ".build_mode", profile.buildMode);
        profile.outputDir = config.GetString(base + ".output_dir", profile.outputDir.generic_string());
        profile.includeDebugSymbols = config.GetBool(base + ".include_debug_symbols", profile.includeDebugSymbols);
    };
    loadOne("client", profiles.client);
    loadOne("server", profiles.server);
    return Result<PackageProfiles>::Ok(std::move(profiles));
}

Status SavePackageProfiles(const PackageProfiles& profiles, const std::filesystem::path& path) {
    Config config;
    auto saveOne = [&config](const char* prefix, const PackageProfile& profile) {
        const std::string base(prefix);
        config.SetBool(base + ".enabled", profile.enabled);
        config.SetString(base + ".preset", profile.preset);
        config.SetString(base + ".build_mode", profile.buildMode);
        config.SetString(base + ".output_dir", profile.outputDir.generic_string());
        config.SetBool(base + ".include_debug_symbols", profile.includeDebugSymbols);
    };
    saveOne("client", profiles.client);
    saveOne("server", profiles.server);
    return config.Save(path);
}

Result<std::filesystem::path> ResolvePackageOutputDir(const std::filesystem::path& projectRoot,
                                                      const std::filesystem::path& outputDir) {
    if (outputDir.empty()) {
        return Result<std::filesystem::path>::Fail("package output directory is empty");
    }
    if (outputDir.is_absolute()) {
        return Result<std::filesystem::path>::Ok(outputDir.lexically_normal());
    }
    const Result<std::string> normalized = NormalizeResourcePath(outputDir.generic_string());
    if (!normalized) {
        return Result<std::filesystem::path>::Fail(normalized.error);
    }
    return Result<std::filesystem::path>::Ok((projectRoot / normalized.value).lexically_normal());
}

} // namespace Hockey
