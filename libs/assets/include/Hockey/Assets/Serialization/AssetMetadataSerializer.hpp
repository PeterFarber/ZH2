#pragma once
#include <filesystem>
#include <string>

#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Reads/writes AssetMetadata to the per-asset "<raw>.meta.yaml" sidecar and to
// in-database YAML nodes. Paths are stored project-relative in generic format.
class AssetMetadataSerializer {
public:
    static std::string Serialize(const AssetMetadata& metadata);
    static bool Deserialize(const std::string& text, AssetMetadata& out);

    static Status SaveSidecar(const std::filesystem::path& sidecarPath,
                              const AssetMetadata& metadata);
    static Result<AssetMetadata> LoadSidecar(const std::filesystem::path& sidecarPath);
};

} // namespace Hockey
