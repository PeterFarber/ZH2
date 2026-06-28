#pragma once

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Result.hpp"
#include <filesystem>
#include <string>
#include <string_view>

namespace Hockey {

struct RuntimeConfigLoadInfo {
    std::string_view embeddedToml;
    std::string_view embeddedSourceName;
    std::string siblingFilename;
    std::filesystem::path commandLineOverride;
};

struct RuntimeConfigLoadResult {
    Config config;
    std::filesystem::path userConfigPath;
    bool loadedUserOverride = false;
    bool usedCommandLineOverride = false;
};

Result<RuntimeConfigLoadResult> LoadRuntimeConfig(const RuntimeConfigLoadInfo& info);
Status SaveRuntimeUserConfig(const Config& config, const std::filesystem::path& path);

} // namespace Hockey
