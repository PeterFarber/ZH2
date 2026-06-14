#include "Hockey/Assets/Assets/ShaderAsset.hpp"

#include <algorithm>

namespace Hockey {

std::string ShaderAssetStageToString(ShaderAssetStage stage) {
    switch (stage) {
    case ShaderAssetStage::Vertex:
        return "Vertex";
    case ShaderAssetStage::Fragment:
        return "Fragment";
    case ShaderAssetStage::Compute:
        return "Compute";
    case ShaderAssetStage::Unknown:
        break;
    }
    return "Unknown";
}

ShaderAssetStage ShaderAssetStageFromString(const std::string& value) {
    if (value == "Vertex")
        return ShaderAssetStage::Vertex;
    if (value == "Fragment")
        return ShaderAssetStage::Fragment;
    if (value == "Compute")
        return ShaderAssetStage::Compute;
    return ShaderAssetStage::Unknown;
}

ShaderAssetStage ShaderStageFromExtension(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".vert")
        return ShaderAssetStage::Vertex;
    if (ext == ".frag")
        return ShaderAssetStage::Fragment;
    if (ext == ".comp")
        return ShaderAssetStage::Compute;
    return ShaderAssetStage::Unknown;
}

} // namespace Hockey
