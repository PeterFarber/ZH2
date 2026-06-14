#include "Hockey/Assets/Assets/TextureAsset.hpp"

namespace Hockey {

std::string TextureColorSpaceToString(TextureColorSpace value) {
    return value == TextureColorSpace::Linear ? "Linear" : "SRGB";
}

TextureColorSpace TextureColorSpaceFromString(const std::string& value) {
    return value == "Linear" ? TextureColorSpace::Linear : TextureColorSpace::SRGB;
}

std::string TextureSemanticToString(TextureSemantic value) {
    switch (value) {
    case TextureSemantic::BaseColor:
        return "BaseColor";
    case TextureSemantic::Normal:
        return "Normal";
    case TextureSemantic::MetallicRoughness:
        return "MetallicRoughness";
    case TextureSemantic::Occlusion:
        return "Occlusion";
    case TextureSemantic::Emissive:
        return "Emissive";
    case TextureSemantic::Height:
        return "Height";
    case TextureSemantic::Mask:
        return "Mask";
    case TextureSemantic::Unknown:
        break;
    }
    return "Unknown";
}

TextureSemantic TextureSemanticFromString(const std::string& value) {
    if (value == "BaseColor")
        return TextureSemantic::BaseColor;
    if (value == "Normal")
        return TextureSemantic::Normal;
    if (value == "MetallicRoughness")
        return TextureSemantic::MetallicRoughness;
    if (value == "Occlusion")
        return TextureSemantic::Occlusion;
    if (value == "Emissive")
        return TextureSemantic::Emissive;
    if (value == "Height")
        return TextureSemantic::Height;
    if (value == "Mask")
        return TextureSemantic::Mask;
    return TextureSemantic::Unknown;
}

} // namespace Hockey
