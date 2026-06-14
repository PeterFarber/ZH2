#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

// Reads/writes the aggregated asset database (data/cooked/asset_database.yaml)
// as a flat list of asset metadata records.
class AssetDatabaseSerializer {
public:
    static std::string Serialize(const std::vector<const AssetMetadata*>& assets);
    static bool Deserialize(const std::string& text, std::vector<AssetMetadata>& out);

    static Status Save(const std::filesystem::path& path,
                       const std::vector<const AssetMetadata*>& assets);
    static Result<std::vector<AssetMetadata>> Load(const std::filesystem::path& path);
};

} // namespace Hockey
