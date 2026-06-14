#include "Hockey/Assets/AssetEvent.hpp"

namespace Hockey {

std::string AssetEventTypeToString(AssetEventType type) {
    switch (type) {
    case AssetEventType::Imported:
        return "Imported";
    case AssetEventType::Cooked:
        return "Cooked";
    case AssetEventType::Modified:
        return "Modified";
    case AssetEventType::Deleted:
        return "Deleted";
    case AssetEventType::Moved:
        return "Moved";
    case AssetEventType::Reloaded:
        return "Reloaded";
    case AssetEventType::Failed:
        return "Failed";
    }
    return "Modified";
}

} // namespace Hockey
