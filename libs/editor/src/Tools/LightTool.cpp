#include "Hockey/Editor/Tools/LightTool.hpp"

#include <string>
#include <vector>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/UndoRedo.hpp"

namespace Hockey {

const char* LightTool::Name() const {
    switch (m_Type) {
    case LightComponent::Type::Point:
        return "Point Light";
    case LightComponent::Type::Spot:
        return "Spot Light";
    case LightComponent::Type::Directional:
    default:
        return "Directional Light";
    }
}

void LightTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    const LightComponent::Type type = m_Type;
    const std::string name = Name();

    context.undoRedo.Execute(EditorCommands::SpawnEntities("Create " + name,
                                                           [type, name](Scene& scene) -> std::vector<UUID> {
                                                               Entity entity = scene.CreateEntity(name);
                                                               LightComponent& light =
                                                                   entity.AddComponent<LightComponent>();
                                                               light.type = type;

                                                               TransformComponent& transform =
                                                                   entity.GetComponent<TransformComponent>();
                                                               if (type == LightComponent::Type::Directional) {
                                                                   transform.localRotation = {50.0f, -30.0f, 0.0f};
                                                               } else {
                                                                   transform.localPosition = {0.0f, 5.0f, 0.0f};
                                                               }
                                                               return {entity.GetUUID()};
                                                           }),
                             context);
}

} // namespace Hockey
