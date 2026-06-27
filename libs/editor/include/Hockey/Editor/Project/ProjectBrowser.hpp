#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Core/Result.hpp"
#include "Hockey/Editor/Project/FileTypeRegistry.hpp"

namespace Hockey {

class AssetDatabase;

// One file or directory entry under a browsed root.
struct ProjectEntry {
    std::filesystem::path path;
    std::string name;
    bool isDirectory = false;
    FileTypeInfo type;
};

// Virtual Project entry backed by cooked asset metadata. Directories are
// synthetic folders derived from cooked assets' rawPath parents.
struct CookedProjectEntry {
    std::string displayName;
    std::filesystem::path virtualFolderPath;
    std::filesystem::path rawPath;
    std::filesystem::path cookedPath;
    AssetID assetId;
    FileTypeInfo type;
    bool isDirectory = false;
};

// Filesystem-backed model for the Project panel. Owns the browse roots, the
// current file selection and the search filter, and performs the on-disk file
// operations (create folder, rename, delete, reveal). Directory contents are
// read on demand by the panel as nodes expand; there is no persistent cache, so
// "refresh" is implicit and explicit refresh simply re-reads.
class ProjectBrowser {
public:
    struct Root {
        std::filesystem::path path;
        std::string label;
    };

    ProjectBrowser();

    const std::vector<Root>& Roots() const {
        return m_Roots;
    }

    // Immediate children of 'dir', directories first then files, alphabetically.
    std::vector<ProjectEntry> Entries(const std::filesystem::path& dir) const;
    // Immediate virtual children of 'folder' from cooked, non-missing metadata.
    std::vector<CookedProjectEntry> Entries(const std::filesystem::path& folder,
                                            const AssetDatabase* database) const;
    std::vector<CookedProjectEntry> SectionEntries(const AssetDatabase* database, AssetType type) const;

    std::filesystem::path RootForPath(const std::filesystem::path& path) const;
    std::filesystem::path ParentFolderWithinRoots(const std::filesystem::path& path) const;
    bool IsWithinRoots(const std::filesystem::path& path) const;

    void Select(const std::filesystem::path& path) {
        m_Selected = path;
    }
    const std::filesystem::path& Selected() const {
        return m_Selected;
    }
    bool HasSelection() const {
        return !m_Selected.empty();
    }
    void ClearSelectionIfMissing();

    // Mutable buffer backing the search input (case-insensitive substring).
    char* SearchBuffer() {
        return m_Search.data();
    }
    int SearchBufferSize() const {
        return static_cast<int>(m_Search.size());
    }
    bool SearchActive() const;
    bool MatchesSearch(const std::string& name) const;

    Status CreateFolder(const std::filesystem::path& parent, const std::string& name);
    Status RenameEntry(const std::filesystem::path& path, const std::string& newName);
    Status DeleteEntry(const std::filesystem::path& path);

    // Best-effort "show in file manager" for the containing directory.
    static void Reveal(const std::filesystem::path& path);
    // Best-effort open with the operating system's default source-file handler.
    static void OpenSourceFile(const std::filesystem::path& path);

private:
    std::vector<Root> m_Roots;
    std::filesystem::path m_Selected;
    std::array<char, 128> m_Search{};
};

} // namespace Hockey
