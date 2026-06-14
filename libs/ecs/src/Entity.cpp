#include "Hockey/ECS/Entity.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

bool Entity::IsValid() const {
    return m_Scene != nullptr && m_Scene->Registry().valid(m_Handle);
}

UUID Entity::GetUUID() const {
    return m_Scene->Registry().get<IDComponent>(m_Handle).id;
}

const std::string& Entity::GetName() const {
    return m_Scene->Registry().get<NameComponent>(m_Handle).name;
}

void Entity::SetName(const std::string& name) {
    auto& nameComponent = m_Scene->Registry().get<NameComponent>(m_Handle);
    if (nameComponent.name == name) {
        return;
    }
    nameComponent.name = name;
    m_Scene->PushEvent({SceneEventType::EntityRenamed, GetUUID(), {}});
}

bool Entity::IsActive() const {
    return m_Scene->Registry().get<ActiveComponent>(m_Handle).active;
}

bool Entity::IsActiveInHierarchy() const {
    return m_Scene->Registry().get<ActiveComponent>(m_Handle).activeInHierarchy;
}

void Entity::SetActive(bool active) {
    m_Scene->SetActive(*this, active);
}

} // namespace Hockey
