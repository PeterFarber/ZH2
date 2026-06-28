#pragma once
#include <filesystem>

#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Assets/Importer.hpp"

namespace Hockey {

class TextureImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override { return AssetType::Texture; }
    std::string Version() const override { return "TextureImporter_v1"; }
    ImportResult Import(const ImportContext& context) override;

    // Infers color space / semantic from the filename (e.g. *_normal -> linear
    // normal map, *_basecolor -> sRGB). Shared with the cooker so import and
    // cook stay consistent without persisting settings separately.
    static TextureImportSettings InferSettings(const std::filesystem::path& rawPath);
    static bool IsSvg(const std::filesystem::path& rawPath);
};

} // namespace Hockey
