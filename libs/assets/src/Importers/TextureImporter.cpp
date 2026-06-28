#include "Hockey/Assets/Importers/TextureImporter.hpp"

#include "Hockey/Assets/Runtime/SvgRasterizer.hpp"
#include "Hockey/Assets/Runtime/TextureLoader.hpp"

#include <algorithm>
#include <cctype>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
bool Contains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}
} // namespace

bool TextureImporter::SupportsExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" ||
           ext == ".hdr" || ext == ".ktx" || ext == ".ktx2" || ext == ".svg";
}

bool TextureImporter::IsSvg(const fs::path& rawPath) {
    std::string ext = rawPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".svg";
}

TextureImportSettings TextureImporter::InferSettings(const fs::path& rawPath) {
    std::string stem = rawPath.stem().string();
    std::transform(stem.begin(), stem.end(), stem.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    TextureImportSettings settings;
    if (IsSvg(rawPath)) {
        settings.semantic = TextureSemantic::BaseColor;
        settings.colorSpace = TextureColorSpace::SRGB;
        settings.normalMap = false;
        settings.generateMipmaps = true;
        settings.compress = true;
        settings.maxSize = SvgRasterizer::kDefaultMaxSize;
        return settings;
    }

    if (Contains(stem, "normal") || Contains(stem, "_n") || Contains(stem, "nrm")) {
        settings.semantic = TextureSemantic::Normal;
        settings.colorSpace = TextureColorSpace::Linear;
        settings.normalMap = true;
    } else if (Contains(stem, "basecolor") || Contains(stem, "albedo") || Contains(stem, "diffuse") ||
               Contains(stem, "base_color")) {
        settings.semantic = TextureSemantic::BaseColor;
        settings.colorSpace = TextureColorSpace::SRGB;
    } else if (Contains(stem, "metallic") || Contains(stem, "roughness") || Contains(stem, "_mr") ||
               Contains(stem, "metalrough")) {
        settings.semantic = TextureSemantic::MetallicRoughness;
        settings.colorSpace = TextureColorSpace::Linear;
    } else if (Contains(stem, "occlusion") || Contains(stem, "_ao") || Contains(stem, "ambient")) {
        settings.semantic = TextureSemantic::Occlusion;
        settings.colorSpace = TextureColorSpace::Linear;
    } else if (Contains(stem, "emissive") || Contains(stem, "emission")) {
        settings.semantic = TextureSemantic::Emissive;
        settings.colorSpace = TextureColorSpace::SRGB;
    } else if (Contains(stem, "height") || Contains(stem, "displace")) {
        settings.semantic = TextureSemantic::Height;
        settings.colorSpace = TextureColorSpace::Linear;
    } else if (Contains(stem, "mask")) {
        settings.semantic = TextureSemantic::Mask;
        settings.colorSpace = TextureColorSpace::Linear;
    }
    return settings;
}

ImportResult TextureImporter::Import(const ImportContext& context) {
    ImportResult result;

    uint32_t width = 0, height = 0, channels = 0;
    if (IsSvg(context.rawPath)) {
        Result<SvgInfo> info = SvgRasterizer::Inspect(context.rawPath);
        if (!info) {
            result.success = false;
            result.error = info.error;
            return result;
        }
        width = info.value.width;
        height = info.value.height;
        channels = 4;
    } else if (!TextureLoader::ReadDimensions(context.rawPath, width, height, channels)) {
        result.success = false;
        result.error = "not a readable image: " + context.rawPath.string();
        return result;
    }

    const TextureImportSettings settings = InferSettings(context.rawPath);

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Texture;
    result.metadata.name = context.rawPath.stem().string();
    // Textures have no asset dependencies of their own.
    result.metadata.dependencies.clear();
    (void)settings;
    return result;
}

} // namespace Hockey
