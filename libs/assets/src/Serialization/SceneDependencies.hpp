#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

class AssetDatabase;

namespace SceneDependencies {

// Walks a scene/prefab YAML document and collects every scalar value that
// matches an existing asset id in `database`. AssetIDs are full 64-bit random
// values, so a stored reference is unambiguous; this keeps hockey_assets
// independent of the ECS component layout. Returns de-duplicated ids.
std::vector<AssetID> Collect(const std::string& yamlText, const AssetDatabase& database);

// Validates that `yamlText` parses as YAML. Returns an error Status otherwise.
Status Validate(const std::string& yamlText);

} // namespace SceneDependencies
} // namespace Hockey
