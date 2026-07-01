#include "Hockey/Assets/AssetType.hpp"

namespace Hockey {

std::string AssetTypeToString(AssetType type) {
    switch (type) {
    case AssetType::Texture:
        return "Texture";
    case AssetType::Mesh:
        return "Mesh";
    case AssetType::Material:
        return "Material";
    case AssetType::Model:
        return "Model";
    case AssetType::Shader:
        return "Shader";
    case AssetType::Scene:
        return "Scene";
    case AssetType::Prefab:
        return "Prefab";
    case AssetType::Audio:
        return "Audio";
    case AssetType::Skeleton:
        return "Skeleton";
    case AssetType::Animation:
        return "Animation";
    case AssetType::Unknown:
        break;
    }
    return "Unknown";
}

AssetType AssetTypeFromString(const std::string& value) {
    if (value == "Texture")
        return AssetType::Texture;
    if (value == "Mesh")
        return AssetType::Mesh;
    if (value == "Material")
        return AssetType::Material;
    if (value == "Model")
        return AssetType::Model;
    if (value == "Shader")
        return AssetType::Shader;
    if (value == "Scene")
        return AssetType::Scene;
    if (value == "Prefab")
        return AssetType::Prefab;
    if (value == "Audio")
        return AssetType::Audio;
    if (value == "Skeleton")
        return AssetType::Skeleton;
    if (value == "Animation")
        return AssetType::Animation;
    return AssetType::Unknown;
}

} // namespace Hockey
