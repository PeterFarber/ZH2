#pragma once

#include <cstdint>

namespace Hockey {

// ImGui drag-drop payload type for an imported content asset. The Project panel
// emits it when dragging an asset row; inspector asset fields, the hierarchy and
// the viewport accept it.
inline constexpr const char* kAssetDragDropType = "HOCKEY_ASSET_ID";

// Payload contents for kAssetDragDropType. `type` holds the AssetType enum value
// as an int so drop targets can filter by category without including the asset
// headers in their payload struct.
struct AssetDragPayload {
    std::uint64_t id = 0;
    int type = 0;
};

} // namespace Hockey
