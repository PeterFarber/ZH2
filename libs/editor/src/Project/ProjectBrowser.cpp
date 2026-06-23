#include "Hockey/Editor/Project/ProjectBrowser.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <system_error>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"

namespace Hockey {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::filesystem::path ResolveProjectPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path.lexically_normal();
    }
    return (Paths::Get().root / path).lexically_normal();
}

std::string ComparablePath(const std::filesystem::path& path) {
    return ToLower(ResolveProjectPath(path).generic_string());
}

bool SamePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    return ComparablePath(lhs) == ComparablePath(rhs);
}

bool PathIsWithin(const std::filesystem::path& path, const std::filesystem::path& root) {
    const std::string value = ComparablePath(path);
    std::string prefix = ComparablePath(root);
    if (value == prefix) {
        return true;
    }
    if (!prefix.empty() && prefix.back() != '/') {
        prefix.push_back('/');
    }
    return !prefix.empty() && value.rfind(prefix, 0) == 0;
}

bool IsVisibleCookedAsset(const AssetMetadata& metadata) {
    return metadata.id.IsValid() && metadata.cooked && !metadata.missing && !metadata.rawPath.empty() &&
           !metadata.cookedPath.empty();
}

FileTypeInfo TypeInfoForAsset(const AssetMetadata& metadata) {
    switch (metadata.type) {
    case AssetType::Texture:
        return {EditorFileType::Image, "Texture", true};
    case AssetType::Material:
        return {EditorFileType::Material, "Material", true};
    case AssetType::Shader:
        return {EditorFileType::ShaderSource, "Shader", true};
    case AssetType::Scene:
        return {EditorFileType::Scene, "Scene", true};
    case AssetType::Prefab:
        return {EditorFileType::Prefab, "Prefab", true};
    case AssetType::Model:
        return {EditorFileType::Model, "Model", true};
    case AssetType::Mesh:
        return {EditorFileType::Model, "Mesh", true};
    default:
        return FileTypeRegistry::Classify(metadata.rawPath);
    }
}

void SortCookedEntries(std::vector<CookedProjectEntry>& entries) {
    std::sort(entries.begin(), entries.end(), [](const CookedProjectEntry& lhs, const CookedProjectEntry& rhs) {
        if (lhs.isDirectory != rhs.isDirectory) {
            return lhs.isDirectory;
        }
        return ToLower(lhs.displayName) < ToLower(rhs.displayName);
    });
}

} // namespace

ProjectBrowser::ProjectBrowser() {
    const EnginePaths& paths = Paths::Get();
    m_Roots.push_back({paths.rawAssets, "Raw Assets (data/raw)"});
    m_Roots.push_back({paths.data / "editor", "Editor (data/editor)"});
    m_Roots.push_back({paths.data / "shaders", "Shaders (data/shaders)"});
}

std::vector<ProjectEntry> ProjectBrowser::Entries(const std::filesystem::path& dir) const {
    std::vector<ProjectEntry> entries;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return entries;
    }
    for (const auto& dirEntry :
         std::filesystem::directory_iterator(dir, std::filesystem::directory_options::skip_permission_denied, ec)) {
        ProjectEntry entry;
        entry.path = dirEntry.path();
        entry.name = dirEntry.path().filename().string();
        entry.isDirectory = dirEntry.is_directory(ec);
        entry.type = entry.isDirectory ? FileTypeInfo{EditorFileType::Folder, "Folder", true}
                                       : FileTypeRegistry::Classify(entry.path);
        entries.push_back(std::move(entry));
    }

    std::sort(entries.begin(), entries.end(), [](const ProjectEntry& lhs, const ProjectEntry& rhs) {
        if (lhs.isDirectory != rhs.isDirectory) {
            return lhs.isDirectory; // directories first
        }
        return ToLower(lhs.name) < ToLower(rhs.name);
    });
    return entries;
}

std::vector<CookedProjectEntry> ProjectBrowser::Entries(const std::filesystem::path& folder,
                                                        const AssetDatabase* database) const {
    std::vector<CookedProjectEntry> entries;
    if (database == nullptr) {
        return entries;
    }

    const std::filesystem::path resolvedFolder = ResolveProjectPath(folder);
    if (!IsWithinRoots(resolvedFolder)) {
        return entries;
    }

    auto addDirectory = [&](const std::filesystem::path& directory) {
        const std::filesystem::path resolvedDirectory = ResolveProjectPath(directory);
        const auto existing = std::find_if(entries.begin(), entries.end(), [&](const CookedProjectEntry& entry) {
            return entry.isDirectory && SamePath(entry.virtualFolderPath, resolvedDirectory);
        });
        if (existing != entries.end()) {
            return;
        }

        CookedProjectEntry entry;
        entry.displayName = resolvedDirectory.filename().string();
        entry.virtualFolderPath = resolvedDirectory;
        entry.rawPath = resolvedDirectory;
        entry.type = {EditorFileType::Folder, "Folder", true};
        entry.isDirectory = true;
        entries.push_back(std::move(entry));
    };

    for (const AssetMetadata* metadata : database->All()) {
        if (metadata == nullptr || !IsVisibleCookedAsset(*metadata)) {
            continue;
        }

        const std::filesystem::path rawPath = ResolveProjectPath(metadata->rawPath);
        if (!PathIsWithin(rawPath, resolvedFolder)) {
            continue;
        }
        const std::filesystem::path parent = rawPath.parent_path().lexically_normal();
        if (SamePath(parent, resolvedFolder)) {
            CookedProjectEntry entry;
            entry.displayName = rawPath.filename().string();
            entry.virtualFolderPath = resolvedFolder;
            entry.rawPath = rawPath;
            entry.cookedPath = ResolveProjectPath(metadata->cookedPath);
            entry.assetId = metadata->id;
            entry.type = TypeInfoForAsset(*metadata);
            entry.isDirectory = false;
            entries.push_back(std::move(entry));
            continue;
        }

        std::filesystem::path relative = rawPath.lexically_relative(resolvedFolder);
        if (!relative.empty()) {
            addDirectory(resolvedFolder / *relative.begin());
        }
    }

    SortCookedEntries(entries);
    return entries;
}

std::filesystem::path ProjectBrowser::RootForPath(const std::filesystem::path& path) const {
    const std::filesystem::path resolvedPath = ResolveProjectPath(path);
    for (const Root& root : m_Roots) {
        const std::filesystem::path resolvedRoot = ResolveProjectPath(root.path);
        if (PathIsWithin(resolvedPath, resolvedRoot)) {
            return resolvedRoot;
        }
    }
    return {};
}

std::filesystem::path ProjectBrowser::ParentFolderWithinRoots(const std::filesystem::path& path) const {
    const std::filesystem::path resolvedPath = ResolveProjectPath(path);
    const std::filesystem::path root = RootForPath(resolvedPath);
    if (root.empty() || SamePath(resolvedPath, root)) {
        return root;
    }

    const std::filesystem::path parent = resolvedPath.parent_path().lexically_normal();
    return PathIsWithin(parent, root) ? parent : root;
}

bool ProjectBrowser::IsWithinRoots(const std::filesystem::path& path) const {
    return !RootForPath(path).empty();
}

void ProjectBrowser::ClearSelectionIfMissing() {
    std::error_code ec;
    if (!m_Selected.empty() && !std::filesystem::exists(m_Selected, ec)) {
        m_Selected.clear();
    }
}

bool ProjectBrowser::SearchActive() const {
    return m_Search[0] != '\0';
}

bool ProjectBrowser::MatchesSearch(const std::string& name) const {
    if (!SearchActive()) {
        return true;
    }
    const std::string needle = ToLower(std::string(m_Search.data()));
    return ToLower(name).find(needle) != std::string::npos;
}

Status ProjectBrowser::CreateFolder(const std::filesystem::path& parent, const std::string& name) {
    if (name.empty()) {
        return Status::Fail("Folder name is empty");
    }
    const std::filesystem::path target = parent / name;
    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        return Status::Fail("A file or folder with that name already exists");
    }
    if (!std::filesystem::create_directory(target, ec) || ec) {
        return Status::Fail("Failed to create folder: " + ec.message());
    }
    HK_EDITOR_INFO("Created folder {}", target.string());
    return Status::Ok();
}

Status ProjectBrowser::RenameEntry(const std::filesystem::path& path, const std::string& newName) {
    if (newName.empty()) {
        return Status::Fail("New name is empty");
    }
    const std::filesystem::path target = path.parent_path() / newName;
    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        return Status::Fail("A file or folder with that name already exists");
    }
    std::filesystem::rename(path, target, ec);
    if (ec) {
        return Status::Fail("Rename failed: " + ec.message());
    }
    if (m_Selected == path) {
        m_Selected = target;
    }
    HK_EDITOR_INFO("Renamed {} -> {}", path.string(), target.string());
    return Status::Ok();
}

Status ProjectBrowser::DeleteEntry(const std::filesystem::path& path) {
    std::error_code ec;
    const std::uintmax_t removed = std::filesystem::remove_all(path, ec);
    if (ec || removed == 0) {
        return Status::Fail("Delete failed: " + (ec ? ec.message() : std::string("nothing removed")));
    }
    if (m_Selected == path) {
        m_Selected.clear();
    }
    HK_EDITOR_INFO("Deleted {}", path.string());
    return Status::Ok();
}

void ProjectBrowser::Reveal(const std::filesystem::path& path) {
    // Best-effort "show in file manager". Opens the containing directory.
    const std::filesystem::path dir = std::filesystem::is_directory(path) ? path : path.parent_path();
#if defined(_WIN32)
    const std::string command = "explorer \"" + dir.string() + "\"";
#elif defined(__APPLE__)
    const std::string command = "open \"" + dir.string() + "\"";
#else
    const std::string command = "xdg-open \"" + dir.string() + "\" >/dev/null 2>&1 &";
#endif
    if (std::system(command.c_str()) != 0) {
        HK_EDITOR_WARN("Could not reveal path {}", dir.string());
    }
}

} // namespace Hockey
