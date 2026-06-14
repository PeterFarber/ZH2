#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetType.hpp"

namespace Hockey {

// Persistent description of a single asset. Stored both in the per-asset
// sidecar (<raw>.meta.yaml) and aggregated in the asset database. Paths are
// project-relative generic-format strings when persisted.
struct AssetMetadata {
    AssetID id;
    AssetType type = AssetType::Unknown;

    std::filesystem::path rawPath;
    std::filesystem::path cookedPath;
    std::filesystem::path metadataPath;

    std::string name;
    std::string importerVersion;
    std::string cookerVersion;

    uint64_t rawFileTimestamp = 0;
    uint64_t rawFileHash = 0;
    uint64_t importedTimestamp = 0;
    uint64_t cookedTimestamp = 0;

    std::vector<AssetID> dependencies;
    std::vector<AssetID> dependents;

    bool dirty = true;
    bool missing = false;
    bool cooked = false;
};

} // namespace Hockey
