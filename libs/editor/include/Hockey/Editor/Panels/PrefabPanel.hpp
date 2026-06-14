#pragma once

#include <string>

#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Prefab authoring panel keyed off the primary selection. Shows prefab-instance
// status (source path / asset id / source entity id), creates a prefab asset
// from the selected entity, and reveals the source file. Override apply/revert
// is disabled because the ECS does not track prefab overrides yet.
class PrefabPanel : public Panel {
public:
    PrefabPanel();
    void OnImGui(EditorContext& context) override;

private:
    char m_NameBuffer[256] = {};
    std::string m_Status;
    bool m_StatusError = false;
};

} // namespace Hockey
