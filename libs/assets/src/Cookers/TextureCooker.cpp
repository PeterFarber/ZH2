#include "Hockey/Assets/Cookers/TextureCooker.hpp"

#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Importers/TextureImporter.hpp"
#include "Hockey/Assets/Runtime/TextureLoader.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

CookResult TextureCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;

    const fs::path rawAbsolute = context.projectRoot / metadata.rawPath;
    const TextureImportSettings settings = TextureImporter::InferSettings(metadata.rawPath);

    Result<TextureAsset> loaded = TextureLoader::LoadRaw(rawAbsolute, settings);
    if (!loaded) {
        result.success = false;
        result.error = loaded.error;
        return result;
    }

    TextureAsset asset = std::move(loaded.value);
    asset.id = metadata.id;

    const uint64_t sourceHash =
        metadata.rawFileHash != 0 ? metadata.rawFileHash : AssetHash::HashFile(rawAbsolute);
    const std::vector<std::byte> cooked = TextureLoader::Encode(asset, sourceHash);

    const fs::path cookedAbsolute = context.cookedRoot / "assets" /
                                    AssetPath::CookedSubdirectory(AssetType::Texture) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Texture));

    std::error_code ec;
    fs::create_directories(cookedAbsolute.parent_path(), ec);
    std::ofstream out(cookedAbsolute, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        result.success = false;
        result.error = "cannot write cooked texture: " + cookedAbsolute.string();
        return result;
    }
    out.write(reinterpret_cast<const char*>(cooked.data()), static_cast<std::streamsize>(cooked.size()));

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey
