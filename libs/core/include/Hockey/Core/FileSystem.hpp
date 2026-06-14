#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include "Hockey/Core/Result.hpp"
namespace Hockey {
class FileSystem {
public:
    static bool Exists(const std::filesystem::path& path);
    static bool IsFile(const std::filesystem::path& path);
    static bool IsDirectory(const std::filesystem::path& path);

    static Status CreateDirectories(const std::filesystem::path& path);
    static Status Remove(const std::filesystem::path& path);

    static Result<std::string> ReadTextFile(const std::filesystem::path& path);
    static Status WriteTextFile(const std::filesystem::path& path, std::string_view text);

    static std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& dir);
    static std::filesystem::path Normalize(const std::filesystem::path& path);
};
}
