#pragma once
#include <filesystem>
#include <string>
namespace Hockey {
struct EnginePaths {
    std::filesystem::path root;
    std::filesystem::path data;
    std::filesystem::path config;
    std::filesystem::path logs;
    std::filesystem::path rawAssets;
    std::filesystem::path cookedAssets;
    std::filesystem::path temp;
};
class Paths {
public:
    static bool Init(const std::filesystem::path& executablePath,
                     const std::filesystem::path& rootOverride = {});

    static const EnginePaths& Get();

    static std::filesystem::path ConfigFile(const std::string& filename);
    static std::filesystem::path LogFile(const std::string& filename);
    static std::filesystem::path DataFile(const std::string& relative);
    static std::filesystem::path RawAsset(const std::string& relative);
    static std::filesystem::path CookedAsset(const std::string& relative);
    static std::filesystem::path TempFile(const std::string& filename);
};
}
