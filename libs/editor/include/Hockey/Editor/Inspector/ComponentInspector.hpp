#pragma once

#include <string>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class EditorContext;
class Entity;

namespace SpawnRoleInspector {

bool IsGoalieSpawn(const Entity& entity);
void SetGoalieSpawn(Entity& entity, bool goalie);

} // namespace SpawnRoleInspector

// Draws every present component on an entity (except the identity/name/active
// components, which the inspector header handles) using the ECS component
// metadata: a collapsing section per component with metadata-driven editable
// fields and a remove action for removable components. Field edits and removals
// are routed through the undo/redo stack: it keeps a "before" snapshot of the
// entity captured the frame an edit starts so one undo step covers a whole drag.
class ComponentInspector {
public:
    void Draw(EditorContext& context, Entity& entity);

private:
    std::string m_PreEditSnapshot;
    UUID m_PreEditEntity{0};
    bool m_Editing = false;
};

} // namespace Hockey
