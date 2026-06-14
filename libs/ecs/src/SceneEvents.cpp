#include "Hockey/ECS/SceneEvents.hpp"

namespace Hockey {

const char* SceneEventTypeToString(SceneEventType type) {
    switch (type) {
    case SceneEventType::EntityCreated:
        return "EntityCreated";
    case SceneEventType::EntityDestroyed:
        return "EntityDestroyed";
    case SceneEventType::ComponentAdded:
        return "ComponentAdded";
    case SceneEventType::ComponentRemoved:
        return "ComponentRemoved";
    case SceneEventType::ParentChanged:
        return "ParentChanged";
    case SceneEventType::EntityRenamed:
        return "EntityRenamed";
    case SceneEventType::ActiveChanged:
        return "ActiveChanged";
    }
    return "Unknown";
}

} // namespace Hockey
