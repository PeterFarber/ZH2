#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Hockey/Editor/Tools/EditorTool.hpp"

namespace Hockey {

class EditorContext;

// Owns the registered editor tools and tracks the persistent active tool.
// Activation routes through here so the toolbar/menu highlight and the viewport
// gizmo stay consistent. Instant tools run their action and leave the active
// tool unchanged.
class ToolManager {
public:
    template <typename T, typename... Args> T& Register(Args&&... args) {
        auto tool = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *tool;
        m_Tools.push_back(std::move(tool));
        return ref;
    }

    const std::vector<std::unique_ptr<EditorTool>>& Tools() const {
        return m_Tools;
    }

    EditorTool* ActiveTool() const {
        return m_Active;
    }
    bool IsActive(const EditorTool* tool) const {
        return m_Active != nullptr && m_Active == tool;
    }

    EditorTool* Find(const std::string& name) const;

    void Activate(EditorTool* tool, EditorContext& context);
    void Activate(const std::string& name, EditorContext& context);

    // Drives per-frame OnUpdate for the active tool.
    void Update(EditorContext& context, float deltaTime);

private:
    std::vector<std::unique_ptr<EditorTool>> m_Tools;
    EditorTool* m_Active = nullptr;
};

} // namespace Hockey
