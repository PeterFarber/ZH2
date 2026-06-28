#include "Hockey/Core/RuntimeConfig.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include <utility>

namespace Hockey {

Result<RuntimeConfigLoadResult> LoadRuntimeConfig(const RuntimeConfigLoadInfo& info) {
    RuntimeConfigLoadResult result;

    if (const Status status = result.config.LoadString(info.embeddedToml, info.embeddedSourceName); !status) {
        return Result<RuntimeConfigLoadResult>::Fail(status.error);
    }

    const bool hasCommandLineOverride = !info.commandLineOverride.empty();
    const std::filesystem::path userPath =
        hasCommandLineOverride ? info.commandLineOverride : Paths::ExecutableSiblingFile(info.siblingFilename);
    result.userConfigPath = userPath;
    result.usedCommandLineOverride = hasCommandLineOverride;

    if (hasCommandLineOverride || FileSystem::Exists(userPath)) {
        Config overlay;
        if (const Status status = overlay.Load(userPath); !status) {
            return Result<RuntimeConfigLoadResult>::Fail(status.error);
        }
        result.config.MergeFrom(overlay);
        result.loadedUserOverride = true;

        if (hasCommandLineOverride) {
            if (const Status status = SaveRuntimeUserConfig(result.config, userPath); !status) {
                return Result<RuntimeConfigLoadResult>::Fail(status.error);
            }
        }
    }

    return Result<RuntimeConfigLoadResult>::Ok(std::move(result));
}

Status SaveRuntimeUserConfig(const Config& config, const std::filesystem::path& path) {
    if (path.empty()) {
        return Status::Fail("cannot save runtime config: empty path");
    }
    return config.Save(path);
}

} // namespace Hockey
