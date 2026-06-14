#include "Hockey/Editor/Tools/TransformTools.hpp"

#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

void TransformTool::OnSelected(EditorContext& context) {
    context.gizmoOperation = m_Operation;
}

} // namespace Hockey
