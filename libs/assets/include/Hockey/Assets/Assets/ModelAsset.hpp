#pragma once
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// A model is the import unit for glTF/GLB files. It owns references to the
// meshes and materials generated during import (textures are referenced
// transitively by the materials).
struct ModelAsset {
    AssetID id;
    std::string name;
    std::vector<AssetID> meshes;
    std::vector<AssetID> materials;
    std::vector<AssetID> skeletons;
    std::vector<AssetID> animations;
};

} // namespace Hockey
