#pragma once
#include <string>
#include <vector>

#include "Hockey/Assets/AssetMetadata.hpp"

namespace Hockey {

// Output of an Importer. `metadata` describes the primary asset; importers that
// fan out (e.g. glTF -> meshes/materials) append extra records to
// `generatedAssets`.
struct ImportResult {
    bool success = false;
    std::string error;
    AssetMetadata metadata;
    std::vector<AssetMetadata> generatedAssets;
};

} // namespace Hockey
