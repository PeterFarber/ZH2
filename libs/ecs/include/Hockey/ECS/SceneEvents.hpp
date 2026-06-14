#pragma once

#include <string>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

enum class SceneEventType {
    EntityCreated,
    EntityDestroyed,
    ComponentAdded,
    ComponentRemoved,
    ParentChanged,
    EntityRenamed,
    ActiveChanged
};

// Lightweight, queryable record of a scene mutation. Lets the future editor/UI
// react to changes without pulling editor code into the ECS layer.
struct SceneEvent {
    SceneEventType type = SceneEventType::EntityCreated;
    UUID entityId{};
    std::string componentName;
};

const char* SceneEventTypeToString(SceneEventType type);

} // namespace Hockey
