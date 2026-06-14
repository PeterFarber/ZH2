#include "Hockey/Editor/FileDialog.hpp"

#include <string>

#include <nfd.h>

#include "Hockey/Core/Log.hpp"

namespace Hockey::FileDialog {

namespace {

// RAII wrapper around NFD_Init/NFD_Quit so each dialog call is self-contained.
struct NfdSession {
    bool ok = false;
    NfdSession() {
        ok = NFD_Init() == NFD_OKAY;
        if (!ok) {
            HK_EDITOR_WARN("Native file dialog unavailable: {}", NFD_GetError());
        }
    }
    ~NfdSession() {
        if (ok) {
            NFD_Quit();
        }
    }
    NfdSession(const NfdSession&) = delete;
    NfdSession& operator=(const NfdSession&) = delete;
};

std::vector<nfdu8filteritem_t> ToNfdFilters(const std::vector<Filter>& filters) {
    std::vector<nfdu8filteritem_t> items;
    items.reserve(filters.size());
    for (const Filter& filter : filters) {
        items.push_back({filter.name, filter.spec});
    }
    return items;
}

} // namespace

std::optional<std::filesystem::path> OpenFile(const std::vector<Filter>& filters,
                                              const std::filesystem::path& defaultDir) {
    NfdSession session;
    if (!session.ok) {
        return std::nullopt;
    }

    const std::vector<nfdu8filteritem_t> items = ToNfdFilters(filters);
    const std::string dir = defaultDir.empty() ? std::string{} : defaultDir.string();

    nfdu8char_t* outPath = nullptr;
    const nfdresult_t result =
        NFD_OpenDialogU8(&outPath, items.empty() ? nullptr : items.data(), static_cast<nfdfiltersize_t>(items.size()),
                         dir.empty() ? nullptr : dir.c_str());
    std::optional<std::filesystem::path> path;
    if (result == NFD_OKAY) {
        path = std::filesystem::path(outPath);
        NFD_FreePathU8(outPath);
    } else if (result == NFD_ERROR) {
        HK_EDITOR_WARN("Open dialog error: {}", NFD_GetError());
    }
    return path;
}

std::optional<std::filesystem::path> SaveFile(const std::vector<Filter>& filters,
                                              const std::filesystem::path& defaultDir, const std::string& defaultName) {
    NfdSession session;
    if (!session.ok) {
        return std::nullopt;
    }

    const std::vector<nfdu8filteritem_t> items = ToNfdFilters(filters);
    const std::string dir = defaultDir.empty() ? std::string{} : defaultDir.string();

    nfdu8char_t* outPath = nullptr;
    const nfdresult_t result =
        NFD_SaveDialogU8(&outPath, items.empty() ? nullptr : items.data(), static_cast<nfdfiltersize_t>(items.size()),
                         dir.empty() ? nullptr : dir.c_str(), defaultName.empty() ? nullptr : defaultName.c_str());
    std::optional<std::filesystem::path> path;
    if (result == NFD_OKAY) {
        path = std::filesystem::path(outPath);
        NFD_FreePathU8(outPath);
    } else if (result == NFD_ERROR) {
        HK_EDITOR_WARN("Save dialog error: {}", NFD_GetError());
    }
    return path;
}

} // namespace Hockey::FileDialog
