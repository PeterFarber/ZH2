#include "Hockey/Editor/Tools/EditorTool.hpp"

namespace Hockey {

// Out-of-line definitions anchor the vtable in this translation unit and provide
// the default no-op behaviour for the optional hooks.
EditorTool::~EditorTool() = default;

void EditorTool::OnSelected(EditorContext& /*context*/) {}
void EditorTool::OnDeselected(EditorContext& /*context*/) {}
void EditorTool::OnUpdate(EditorContext& /*context*/, float /*deltaTime*/) {}
void EditorTool::OnViewportImGui(EditorContext& /*context*/) {}

} // namespace Hockey
