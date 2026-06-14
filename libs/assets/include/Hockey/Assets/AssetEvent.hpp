#pragma once
#include <filesystem>
#include <string>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

enum class AssetEventType { Imported, Cooked, Modified, Deleted, Moved, Reloaded, Failed };

struct AssetEvent {
    AssetEventType type = AssetEventType::Modified;
    AssetID id;
    std::filesystem::path path;
    std::string message;
};

std::string AssetEventTypeToString(AssetEventType type);

} // namespace Hockey
