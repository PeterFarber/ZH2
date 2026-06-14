#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

struct PrefabAsset {
    AssetID id;
    std::string name;
    std::filesystem::path sourcePath;
    std::vector<AssetID> dependencies;
};

} // namespace Hockey
