#include "Hockey/Editor/Project/EditorAssetPreview.hpp"

#include <array>
#include <cstdio>
#include <system_error>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Editor/Project/FileTypeRegistry.hpp"

namespace Hockey::EditorAssetPreview {

namespace {

constexpr std::uintmax_t kSnippetMaxBytes = 4U * 1024U; // only preview small text files
constexpr std::size_t kSnippetMaxChars = 600;

std::string HumanSize(std::uintmax_t bytes) {
    constexpr std::array<const char*, 4> units{"B", "KB", "MB", "GB"};
    auto value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size()) {
        value /= 1024.0;
        ++unit;
    }
    std::array<char, 32> buffer{};
    if (unit == 0) {
        std::snprintf(buffer.data(), buffer.size(), "%llu %s", static_cast<unsigned long long>(bytes), units[unit]);
    } else {
        std::snprintf(buffer.data(), buffer.size(), "%.1f %s", value, units[unit]);
    }
    return std::string(buffer.data());
}

bool IsTextLike(EditorFileType type) {
    switch (type) {
    case EditorFileType::Scene:
    case EditorFileType::Prefab:
    case EditorFileType::Toml:
    case EditorFileType::ShaderSource:
    case EditorFileType::Text:
        return true;
    default:
        return false;
    }
}

} // namespace

Preview Describe(const std::filesystem::path& path) {
    Preview preview;
    const FileTypeInfo info = FileTypeRegistry::Classify(path);
    preview.typeLabel = info.label;

    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    preview.sizeLabel = ec ? "-" : HumanSize(size);

    if (!ec && size <= kSnippetMaxBytes && IsTextLike(info.type)) {
        if (const Result<std::string> text = FileSystem::ReadTextFile(path); text) {
            preview.snippet = text.value.substr(0, kSnippetMaxChars);
            preview.hasSnippet = !preview.snippet.empty();
        }
    }
    return preview;
}

} // namespace Hockey::EditorAssetPreview
