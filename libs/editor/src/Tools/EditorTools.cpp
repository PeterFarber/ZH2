#include "Hockey/Editor/Tools/EditorTools.hpp"

#include "Hockey/Editor/Tools/HockeyTools.hpp"
#include "Hockey/Editor/Tools/LightTool.hpp"
#include "Hockey/Editor/Tools/ToolManager.hpp"
#include "Hockey/Editor/Tools/TransformTools.hpp"

namespace Hockey {

void RegisterEditorTools(ToolManager& tools) {
    // Transform tools (persistent; drive the viewport gizmo).
    tools.Register<SelectTool>();
    tools.Register<MoveTool>();
    tools.Register<RotateTool>();
    tools.Register<ScaleTool>();

    // Lights (instant create).
    tools.Register<LightTool>(LightComponent::Type::Directional);
    tools.Register<LightTool>(LightComponent::Type::Point);
    tools.Register<LightTool>(LightComponent::Type::Spot);

    // Hockey markers (instant create).
    tools.Register<HockeyRinkTool>();
    tools.Register<HockeySpawnTool>();
    tools.Register<HockeyGoalTool>();
    tools.Register<HockeyPuckTool>();
    tools.Register<HockeyFaceoffTool>();
    tools.Register<HockeyCameraRigTool>();
}

} // namespace Hockey
