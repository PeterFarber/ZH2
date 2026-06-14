#include "Hockey/Editor/Tools/ToolManager.hpp"

#include <string>

namespace Hockey {

EditorTool* ToolManager::Find(const std::string& name) const {
    for (const std::unique_ptr<EditorTool>& tool : m_Tools) {
        if (name == tool->Name()) {
            return tool.get();
        }
    }
    return nullptr;
}

void ToolManager::Activate(EditorTool* tool, EditorContext& context) {
    if (tool == nullptr) {
        return;
    }
    if (tool->IsInstant()) {
        // One-shot action: run it without disturbing the active persistent tool.
        tool->OnSelected(context);
        tool->OnDeselected(context);
        return;
    }
    if (m_Active == tool) {
        return;
    }
    if (m_Active != nullptr) {
        m_Active->OnDeselected(context);
    }
    m_Active = tool;
    m_Active->OnSelected(context);
}

void ToolManager::Activate(const std::string& name, EditorContext& context) {
    Activate(Find(name), context);
}

void ToolManager::Update(EditorContext& context, float deltaTime) {
    if (m_Active != nullptr) {
        m_Active->OnUpdate(context, deltaTime);
    }
}

} // namespace Hockey
