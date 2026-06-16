#include "Hockey/Core/Paths.hpp"
#include <system_error>
namespace Hockey {
namespace fs = std::filesystem;
namespace {
EnginePaths g_Paths;

fs::path ResolveRoot(const fs::path& executablePath, const fs::path& rootOverride) {
    std::error_code ec;
    if (!rootOverride.empty()) {
        fs::path root = fs::absolute(rootOverride, ec);
        return (ec ? rootOverride : root).lexically_normal();
    }
    // Infer the project root by walking up from the executable directory and
    // looking for a recognizable marker (the data folder or vcpkg manifest).
    fs::path exeDir = executablePath.parent_path();
    for (fs::path candidate = exeDir; !candidate.empty(); candidate = candidate.parent_path()) {
        if (fs::exists(candidate / "data", ec) || fs::exists(candidate / "vcpkg.json", ec)) {
            return candidate.lexically_normal();
        }
        if (candidate == candidate.root_path()) {
            break;
        }
    }
    return (exeDir.empty() ? fs::current_path(ec) : exeDir).lexically_normal();
}
}
bool Paths::Init(const fs::path& executablePath, const fs::path& rootOverride) {
    const fs::path root = ResolveRoot(executablePath, rootOverride);

    g_Paths.root = root;
    g_Paths.data = root / "data";
    g_Paths.config = g_Paths.data / "config";
    g_Paths.logs = g_Paths.data / "logs";
    g_Paths.rawAssets = g_Paths.data / "raw";
    g_Paths.cookedAssets = g_Paths.data / "cooked";
    g_Paths.temp = g_Paths.data / "temp";

    std::error_code ec;
    fs::create_directories(g_Paths.logs, ec);
    fs::create_directories(g_Paths.temp, ec);

    return true;
}
const EnginePaths& Paths::Get() { return g_Paths; }
fs::path Paths::ConfigFile(const std::string& filename) { return g_Paths.config / filename; }
fs::path Paths::LogFile(const std::string& filename) { return g_Paths.logs / filename; }
fs::path Paths::DataFile(const std::string& relative) { return g_Paths.data / relative; }
fs::path Paths::RawAsset(const std::string& relative) { return g_Paths.rawAssets / relative; }
fs::path Paths::CookedAsset(const std::string& relative) { return g_Paths.cookedAssets / relative; }
fs::path Paths::TempFile(const std::string& filename) { return g_Paths.temp / filename; }
fs::path Paths::Resolve(const fs::path& pathOrRelative) {
    if (pathOrRelative.is_absolute()) {
        return pathOrRelative.lexically_normal();
    }
    return (g_Paths.root / pathOrRelative).lexically_normal();
}
}
