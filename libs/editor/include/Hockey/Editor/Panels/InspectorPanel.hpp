#pragma once

#include <string>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/Inspector/AssetInspector.hpp"
#include "Hockey/Editor/Inspector/ComponentAddMenu.hpp"
#include "Hockey/Editor/Inspector/ComponentInspector.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

class Entity;

// Metadata-driven component inspector for the primary selection. Draws the
// entity header (active toggle, editable name, read-only UUID), each component's
// editable fields, and the Add Component menu. Header/name/component edits are
// pushed onto the undo/redo stack.
class InspectorPanel : public Panel {
public:
    InspectorPanel();
    void OnImGui(EditorContext& context) override;

private:
    void DrawLockToggle(EditorContext& context);
    void DrawHeader(EditorContext& context, Entity& entity);

    AssetInspector m_AssetInspector;
    ComponentInspector m_Inspector;
    ComponentAddMenu m_AddMenu;
    bool m_InspectorLocked = false;
    AssetID m_LockedAssetId;
    UUID m_LockedEntityId{0};
    char m_NameBuffer[256] = {};
    char m_TagBuffer[128] = {};
    char m_LayerBuffer[128] = {};
    std::string m_NameOriginal;
    bool m_NameEditing = false;
};

} // namespace Hockey
