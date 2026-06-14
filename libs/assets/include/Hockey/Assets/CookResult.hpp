#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

// Output of a Cooker. `cookedPath` is project-relative; `dependencies` lists
// other assets discovered while cooking (e.g. textures referenced by a material
// or includes pulled in by a shader).
struct CookResult {
    bool success = false;
    std::string error;
    std::filesystem::path cookedPath;
    std::vector<AssetID> dependencies;
};

} // namespace Hockey
