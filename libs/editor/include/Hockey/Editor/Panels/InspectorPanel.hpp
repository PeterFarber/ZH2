#pragma once

#include <string>

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
    void DrawHeader(EditorContext& context, Entity& entity);

    ComponentInspector m_Inspector;
    ComponentAddMenu m_AddMenu;
    char m_NameBuffer[256] = {};
    std::string m_NameOriginal;
    bool m_NameEditing = false;
};

} // namespace Hockey
