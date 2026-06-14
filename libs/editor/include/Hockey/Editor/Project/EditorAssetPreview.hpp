#pragma once

#include <filesystem>
#include <string>

namespace Hockey {

// Lightweight, text-only asset preview for the Project panel's details pane.
// Phase 4 does not import or decode assets, so previews are limited to type,
// size and (for small text assets) a short head of the file content.
namespace EditorAssetPreview {

struct Preview {
    std::string typeLabel;
    std::string sizeLabel; // human-readable file size
    std::string snippet;   // first lines of small text assets, else empty
    bool hasSnippet = false;
};

Preview Describe(const std::filesystem::path& path);

} // namespace EditorAssetPreview

} // namespace Hockey
