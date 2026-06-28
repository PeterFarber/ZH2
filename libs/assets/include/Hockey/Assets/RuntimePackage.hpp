#pragma once

#include "Hockey/Core/ResourceProvider.hpp"
#include "Hockey/Core/Result.hpp"

#include <filesystem>
#include <vector>

namespace Hockey {

enum class RuntimePackageTarget {
    Client,
    Server,
};

struct RuntimePackageBuildInfo {
    RuntimePackageTarget target = RuntimePackageTarget::Client;
    std::filesystem::path projectRoot;
    std::filesystem::path configPath;
};

Result<std::vector<ResourcePackageEntry>> BuildRuntimePackageEntries(const RuntimePackageBuildInfo& info);
Status WriteRuntimePackage(const RuntimePackageBuildInfo& info, const std::filesystem::path& outputPath);

} // namespace Hockey
