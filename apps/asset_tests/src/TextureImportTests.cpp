#include "Test.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Importers/TextureImporter.hpp"
#include "Hockey/Assets/Runtime/TextureLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

// Writes a tiny uncompressed 32-bit TGA (BGRA on disk) that stb_image can
// decode. Pixels are given as RGBA and converted to TGA's BGRA order.
void WriteTga(const fs::path& path, uint16_t width, uint16_t height, const std::vector<uint8_t>& rgba) {
    FileSystem::CreateDirectories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    uint8_t header[18] = {};
    header[2] = 2; // uncompressed true-color
    header[12] = static_cast<uint8_t>(width & 0xFF);
    header[13] = static_cast<uint8_t>((width >> 8) & 0xFF);
    header[14] = static_cast<uint8_t>(height & 0xFF);
    header[15] = static_cast<uint8_t>((height >> 8) & 0xFF);
    header[16] = 32;   // bits per pixel
    header[17] = 0x28; // top-left origin, 8 alpha bits
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    for (size_t i = 0; i + 3 < rgba.size(); i += 4) {
        const uint8_t bgra[4] = {rgba[i + 2], rgba[i + 1], rgba[i + 0], rgba[i + 3]};
        out.write(reinterpret_cast<const char*>(bgra), 4);
    }
}

} // namespace

void RunTextureImportTests() {
    HockeyTest::BeginSuite("TextureImportTests");

    const fs::path workspace = Paths::TempFile("texture_ws");
    FileSystem::Remove(workspace);
    const fs::path texDir = workspace / "data" / "raw" / "textures";

    // 2x2 image: red, green, blue, white.
    const std::vector<uint8_t> pixels = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    WriteTga(texDir / "ice_basecolor.tga", 2, 2, pixels);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");

    HK_CHECK_MSG(static_cast<bool>(manager.DiscoverRawAssets()), "discover ok");
    HK_CHECK_MSG(static_cast<bool>(manager.ImportAll()), "import all ok");

    AssetMetadata* meta = manager.Database().FindByRawPath("data/raw/textures/ice_basecolor.tga");
    HK_CHECK_MSG(meta != nullptr, "texture metadata created");
    HK_CHECK_MSG(meta != nullptr && meta->type == AssetType::Texture, "type is texture");

    // Color space inference from filename.
    const TextureImportSettings baseColorSettings =
        TextureImporter::InferSettings("data/raw/textures/ice_basecolor.tga");
    HK_CHECK_MSG(baseColorSettings.colorSpace == TextureColorSpace::SRGB, "basecolor is sRGB");
    const TextureImportSettings normalSettings = TextureImporter::InferSettings("data/raw/textures/wall_normal.png");
    HK_CHECK_MSG(normalSettings.colorSpace == TextureColorSpace::Linear, "normal is linear");
    HK_CHECK_MSG(normalSettings.normalMap, "normal flagged as normal map");

    // Cook all dirty, then load the cooked texture back.
    HK_CHECK_MSG(static_cast<bool>(manager.CookAllDirty()), "cook all dirty ok");
    meta = manager.Database().FindByRawPath("data/raw/textures/ice_basecolor.tga");
    HK_CHECK_MSG(meta != nullptr && meta->cooked, "texture cooked");
    HK_CHECK_MSG(meta != nullptr && !meta->dirty, "texture no longer dirty");
    HK_CHECK_MSG(meta != nullptr && FileSystem::Exists(workspace / meta->cookedPath), "cooked file exists");

    const AssetID id = meta->id;
    Result<std::shared_ptr<TextureAsset>> loaded = manager.Load<TextureAsset>(id);
    HK_CHECK_MSG(static_cast<bool>(loaded), "load cooked texture");
    HK_CHECK_MSG(loaded && loaded.value->width == 2, "loaded width 2");
    HK_CHECK_MSG(loaded && loaded.value->height == 2, "loaded height 2");
    HK_CHECK_MSG(loaded && loaded.value->colorSpace == TextureColorSpace::SRGB, "loaded color space sRGB");
    HK_CHECK_MSG(loaded && loaded.value->mipLevels >= 2, "mip chain generated");

    // Second load is served from the registry cache (same pointer).
    Result<std::shared_ptr<TextureAsset>> again = manager.Load<TextureAsset>(id);
    HK_CHECK_MSG(again && loaded && again.value.get() == loaded.value.get(), "cached load");

    // Missing/invalid texture reports an error.
    Result<std::shared_ptr<TextureAsset>> missing = manager.Load<TextureAsset>(AssetID(999999999ull));
    HK_CHECK_MSG(!missing, "missing texture load fails");

    // Decoding garbage fails cleanly.
    const std::byte garbage[8] = {};
    HK_CHECK_MSG(!TextureLoader::Decode(garbage, sizeof(garbage)), "garbage decode fails");

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
