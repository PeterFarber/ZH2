#pragma once

namespace Hockey {

// Dear ImGui drag/drop payload identifier used to carry a prefab file path
// (UTF-8, null-terminated) from the Project panel to drop targets (Hierarchy
// panel, Scene viewport). Kept in one place so source and targets agree.
inline constexpr const char* kPrefabDragDropType = "HOCKEY_PREFAB_PATH";

} // namespace Hockey
