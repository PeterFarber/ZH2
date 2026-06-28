#include "Hockey/Assets/RuntimePackage.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <system_error>

namespace Hockey {
namespace {

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

Result<std::string> ProjectRelativeResourcePath(const std::filesystem::path& projectRoot,
                                                const std::filesystem::path& absolutePath) {
    std::error_code ec;
    const std::filesystem::path root = std::filesystem::weakly_canonical(projectRoot, ec);
    if (ec) {
        return Result<std::string>::Fail("cannot canonicalize project root: " + projectRoot.string());
    }
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(absolutePath, ec);
    if (ec) {
        return Result<std::string>::Fail("cannot canonicalize package resource: " + absolutePath.string());
    }
    const std::filesystem::path relative = std::filesystem::relative(canonical, root, ec);
    if (ec || relative.empty()) {
        return Result<std::string>::Fail("cannot make package resource project-relative: " + absolutePath.string());
    }
    const Result<std::string> normalized = NormalizeResourcePath(relative.generic_string());
    if (!normalized) {
        return normalized;
    }
    if (!StartsWith(normalized.value, "data/")) {
        return Result<std::string>::Fail("package resource must be under data/: " + normalized.value);
    }
    return normalized;
}

Result<std::vector<std::byte>> ReadBinaryFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<std::vector<std::byte>>::Fail("cannot open package resource: " + path.string());
    }
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        return Result<std::vector<std::byte>>::Fail("cannot determine package resource size: " + path.string());
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream.good() && !stream.eof()) {
        return Result<std::vector<std::byte>>::Fail("failed while reading package resource: " + path.string());
    }
    return Result<std::vector<std::byte>>::Ok(std::move(bytes));
}

Status AddFile(const std::filesystem::path& projectRoot, const std::filesystem::path& absolutePath,
               std::vector<ResourcePackageEntry>& entries, std::set<std::string>& seen) {
    if (!FileSystem::IsFile(absolutePath)) {
        return Status::Fail("required package resource missing: " + absolutePath.string());
    }
    Result<std::string> relative = ProjectRelativeResourcePath(projectRoot, absolutePath);
    if (!relative) {
        return Status::Fail(relative.error);
    }
    if (!seen.insert(relative.value).second) {
        return Status::Ok();
    }
    Result<std::vector<std::byte>> bytes = ReadBinaryFile(absolutePath);
    if (!bytes) {
        return Status::Fail(bytes.error);
    }
    entries.push_back({relative.value, std::move(bytes.value)});
    return Status::Ok();
}

Status AddRelativeFile(const std::filesystem::path& projectRoot, const std::filesystem::path& relativePath,
                       std::vector<ResourcePackageEntry>& entries, std::set<std::string>& seen) {
    const Result<std::string> normalized = NormalizeResourcePath(relativePath.generic_string());
    if (!normalized) {
        return Status::Fail(normalized.error);
    }
    if (!StartsWith(normalized.value, "data/")) {
        return Status::Fail("package resource must be under data/: " + normalized.value);
    }
    return AddFile(projectRoot, projectRoot / normalized.value, entries, seen);
}

Status AddDirectoryIfPresent(const std::filesystem::path& projectRoot, const std::filesystem::path& relativeDir,
                             std::vector<ResourcePackageEntry>& entries, std::set<std::string>& seen) {
    const std::filesystem::path dir = projectRoot / relativeDir;
    if (!FileSystem::IsDirectory(dir)) {
        return Status::Ok();
    }

    std::error_code ec;
    std::vector<std::filesystem::path> files;
    for (const auto& item : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (ec) {
            return Status::Fail("failed while scanning package directory: " + dir.string() + ": " + ec.message());
        }
        if (item.is_regular_file(ec)) {
            files.push_back(item.path());
        }
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        if (const Status added = AddFile(projectRoot, file, entries, seen); !added) {
            return added;
        }
    }
    return Status::Ok();
}

Status AddConfiguredPath(const std::filesystem::path& projectRoot, const Config& config, const char* key,
                         std::vector<ResourcePackageEntry>& entries, std::set<std::string>& seen) {
    const std::string path = config.GetString(key, "");
    if (path.empty()) {
        return Status::Ok();
    }
    return AddRelativeFile(projectRoot, path, entries, seen);
}

} // namespace

Result<std::vector<ResourcePackageEntry>> BuildRuntimePackageEntries(const RuntimePackageBuildInfo& info) {
    if (info.projectRoot.empty()) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("runtime package project root is empty");
    }
    if (info.configPath.empty()) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("runtime package config path is empty");
    }

    Config config;
    const std::filesystem::path resolvedConfig =
        info.configPath.is_absolute() ? info.configPath : (info.projectRoot / info.configPath);
    if (const Status loaded = config.Load(resolvedConfig); !loaded) {
        return Result<std::vector<ResourcePackageEntry>>::Fail(loaded.error);
    }

    std::vector<ResourcePackageEntry> entries;
    std::set<std::string> seen;
    if (const Status added = AddFile(info.projectRoot, resolvedConfig, entries, seen); !added) {
        return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
    }
    if (const Status added = AddRelativeFile(info.projectRoot, "data/gameplay/tuning.default.yaml", entries, seen);
        !added) {
        return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
    }
    if (const Status added = AddConfiguredPath(info.projectRoot, config, "scene.startup_scene", entries, seen);
        !added) {
        return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
    }
    if (const Status added = AddConfiguredPath(info.projectRoot, config, "gameplay.waypoint_prefab_path", entries, seen);
        !added) {
        return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
    }

    if (info.target == RuntimePackageTarget::Client) {
        if (const Status added = AddDirectoryIfPresent(info.projectRoot, "data/ui", entries, seen); !added) {
            return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
        }
        if (const Status added = AddDirectoryIfPresent(info.projectRoot, "data/shaders/bin", entries, seen); !added) {
            return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
        }
        if (const Status added = AddDirectoryIfPresent(info.projectRoot, "data/cooked", entries, seen); !added) {
            return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
        }
    } else {
        if (FileSystem::Exists(info.projectRoot / "data/cooked/asset_database.yaml")) {
            if (const Status added =
                    AddRelativeFile(info.projectRoot, "data/cooked/asset_database.yaml", entries, seen);
                !added) {
                return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
            }
        }
        for (const char* dir : {"data/cooked/assets/scenes", "data/cooked/assets/prefabs", "data/cooked/assets/meshes"}) {
            if (const Status added = AddDirectoryIfPresent(info.projectRoot, dir, entries, seen); !added) {
                return Result<std::vector<ResourcePackageEntry>>::Fail(added.error);
            }
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const ResourcePackageEntry& a, const ResourcePackageEntry& b) { return a.path < b.path; });
    return Result<std::vector<ResourcePackageEntry>>::Ok(std::move(entries));
}

Status WriteRuntimePackage(const RuntimePackageBuildInfo& info, const std::filesystem::path& outputPath) {
    Result<std::vector<ResourcePackageEntry>> entries = BuildRuntimePackageEntries(info);
    if (!entries) {
        return Status::Fail(entries.error);
    }
    return WriteResourcePackageFile(outputPath, std::move(entries.value));
}

} // namespace Hockey
