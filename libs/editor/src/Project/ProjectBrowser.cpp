#include "Hockey/Editor/Project/ProjectBrowser.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <system_error>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"

namespace Hockey {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
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
