#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Scene/prefab assets are tracked (and cooked) primarily for their dependency
// edges to meshes/materials/textures/prefabs. The cooked artifact is validated
// YAML; ECS owns the actual scene runtime representation.
struct SceneAsset {
    AssetID id;
    std::string name;
    std::filesystem::path sourcePath;
    std::vector<AssetID> dependencies;
};

} // namespace Hockey
