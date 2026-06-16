#pragma once

#include <string>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

class EditorContext;
class Entity;

// Prefab authoring panel keyed off the primary selection. Shows prefab-instance
// status (source path / asset id / source entity id), creates a prefab asset
// from the selected entity, reveals the source file, and applies/reverts the
// instance's overrides against its linked prefab.
class PrefabPanel : public Panel {
public:
    PrefabPanel();
    void OnImGui(EditorContext& context) override;

private:
    // Recomputes the cached override count for `entity` (a prefab instance).
    void RefreshOverrides(EditorContext& context, Entity entity);

    char m_NameBuffer[256] = {};
    std::string m_Status;
    bool m_StatusError = false;

    // Cached prefab-override diff for the currently inspected instance, so the
    // prefab file is only re-read on selection change or explicit refresh.
    UUID m_OverrideEntityId{0};
    int m_OverrideCount = 0;
    bool m_OverrideValid = false;
};

} // namespace Hockey
