#include "Hockey/Editor/Selection.hpp"

#include <algorithm>

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

void Selection::Select(UUID entityId) {
    m_Selected.clear();
    if (entityId.IsValid()) {
        m_Selected.push_back(entityId);
        m_Primary = entityId;
    } else {
        m_Primary = UUID(0);
    }
}

void Selection::SelectAll(Scene& scene) {
    m_Selected.clear();
    m_Primary = UUID(0);
    for (Entity entity : scene.GetAllEntities()) {
        const UUID id = entity.GetUUID();
        if (id.IsValid()) {
            m_Selected.push_back(id);
            m_Primary = id;
        }
    }
}

void Selection::Add(UUID entityId) {
    if (!entityId.IsValid()) {
        return;
    }
    if (!IsSelected(entityId)) {
        m_Selected.push_back(entityId);
    }
    m_Primary = entityId;
}

void Selection::Remove(UUID entityId) {
    const auto it = std::find(m_Selected.begin(), m_Selected.end(), entityId);
    if (it != m_Selected.end()) {
        m_Selected.erase(it);
    }
    if (m_Primary == entityId) {
        m_Primary = m_Selected.empty() ? UUID(0) : m_Selected.back();
    }
}

void Selection::Toggle(UUID entityId) {
    if (!entityId.IsValid()) {
        return;
    }
    if (IsSelected(entityId)) {
        Remove(entityId);
    } else {
        Add(entityId);
    }
}

void Selection::Clear() {
    m_Selected.clear();
    m_Primary = UUID(0);
}

bool Selection::IsSelected(UUID entityId) const {
    return std::find(m_Selected.begin(), m_Selected.end(), entityId) != m_Selected.end();
}

UUID Selection::Primary() const {
    return m_Primary;
}

const std::vector<UUID>& Selection::All() const {
    return m_Selected;
}

std::size_t Selection::Count() const {
    return m_Selected.size();
}

bool Selection::Empty() const {
    return m_Selected.empty();
}

void Selection::Validate(const Scene& scene) {
    m_Selected.erase(std::remove_if(m_Selected.begin(), m_Selected.end(),
                                    [&scene](const UUID& id) { return !scene.ContainsUUID(id); }),
                     m_Selected.end());
    if (!m_Primary.IsValid() || !scene.ContainsUUID(m_Primary)) {
        m_Primary = m_Selected.empty() ? UUID(0) : m_Selected.back();
    }
}

} // namespace Hockey
