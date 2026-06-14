#include "Hockey/Core/FileSystem.hpp"
#include <fstream>
#include <sstream>
#include <system_error>
namespace Hockey {
namespace fs = std::filesystem;
bool FileSystem::Exists(const fs::path& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}
bool FileSystem::IsFile(const fs::path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}
bool FileSystem::IsDirectory(const fs::path& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}
Status FileSystem::CreateDirectories(const fs::path& path) {
    if (path.empty() || Exists(path)) {
        return Status::Ok();
    }
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        return Status::Fail("failed to create directories '" + path.string() + "': " + ec.message());
    }
    return Status::Ok();
}
Status FileSystem::Remove(const fs::path& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    if (ec) {
        return Status::Fail("failed to remove '" + path.string() + "': " + ec.message());
    }
    return Status::Ok();
}
Result<std::string> FileSystem::ReadTextFile(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return Result<std::string>::Fail("cannot open file for read: " + path.string());
    }
    std::ostringstream contents;
    contents << stream.rdbuf();
    return Result<std::string>::Ok(contents.str());
}
Status FileSystem::WriteTextFile(const fs::path& path, std::string_view text) {
    if (path.has_parent_path()) {
        const Status dirs = CreateDirectories(path.parent_path());
        if (!dirs) {
            return dirs;
        }
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return Status::Fail("cannot open file for write: " + path.string());
    }
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream.good()) {
        return Status::Fail("failed while writing file: " + path.string());
    }
    return Status::Ok();
}
std::vector<fs::path> FileSystem::ListFiles(const fs::path& dir) {
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        return files;
    }
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    return files;
}
fs::path FileSystem::Normalize(const fs::path& path) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(path, ec);
    if (ec || canonical.empty()) {
        return path.lexically_normal();
    }
    return canonical;
}
}
