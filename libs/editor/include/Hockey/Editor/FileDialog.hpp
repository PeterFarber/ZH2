#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace Hockey::FileDialog {

// One file-type filter for a native dialog. 'spec' is a comma-separated list of
// extensions without dots, e.g. "yaml" or "png,jpg".
struct Filter {
    const char* name;
    const char* spec;
};

// Native "open file" dialog. Returns the chosen path, or nullopt if the user
// cancelled or no native dialog is available (e.g. headless).
std::optional<std::filesystem::path> OpenFile(const std::vector<Filter>& filters,
                                              const std::filesystem::path& defaultDir = {});

// Native "save file" dialog. 'defaultName' pre-fills the filename field.
std::optional<std::filesystem::path> SaveFile(const std::vector<Filter>& filters,
                                              const std::filesystem::path& defaultDir = {},
                                              const std::string& defaultName = {});

} // namespace Hockey::FileDialog
