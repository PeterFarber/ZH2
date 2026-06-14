#include "Hockey/ECS/Hierarchy.hpp"

#include <algorithm>

#include <glm/glm.hpp>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/Transform.hpp"

namespace Hockey {

namespace Hierarchy {

bool IsDescendantOf(const Scene& scene, Entity child, Entity possibleParent) {
    if (!child.IsValid() || !possibleParent.IsValid()) {
        return false;
    }
    Entity current = scene.GetParent(child);
    while (current.IsValid()) {
        if (current == possibleParent) {
            return true;
        }
        current = scene.GetParent(current);
    }
    return false;
}

} // namespace Hierarchy

bool Scene::IsDescendantOf(Entity child, Entity possibleParent) const {
    return Hierarchy::IsDescendantOf(*this, child, possibleParent);
}

void Scene::SetParent(Entity child, Entity parent, bool keepWorldTransform) {
    if (!IsValid(child) || !IsValid(parent)) {
        HK_CORE_WARN("SetParent: invalid child or parent");
        return;
    }
    if (child == parent) {
        HK_CORE_WARN("SetParent: an entity cannot be parented to itself");
        return;
    }
    if (IsDescendantOf(parent, child)) {
        HK_CORE_WARN("SetParent: cannot parent an entity to its own descendant");
        return;
    }

    const UUID childId = m_Registry.get<IDComponent>(child.Handle()).id;
    const UUID parentId = m_Registry.get<IDComponent>(parent.Handle()).id;

    if (const auto* existing = m_Registry.try_get<ParentComponent>(child.Handle());
        existing != nullptr && existing->parentId == parentId) {
        return;
    }

    glm::mat4 oldWorld(1.0f);
    if (keepWorldTransform) {
        oldWorld = WorldMatrix(child.Handle());
    }

    RemoveFromParentChildList(child.Handle());
    m_Registry.emplace_or_replace<ParentComponent>(child.Handle(), ParentComponent{parentId});

    auto& parentChildren = m_Registry.get<ChildrenComponent>(parent.Handle()).children;
    if (std::find(parentChildren.begin(), parentChildren.end(), childId) == parentChildren.end()) {
        parentChildren.push_back(childId);
    }

    if (keepWorldTransform) {
        const glm::mat4 parentWorld = WorldMatrix(parent.Handle());
        const glm::mat4 newLocal = glm::inverse(parentWorld) * oldWorld;
        auto& transform = m_Registry.get<TransformComponent>(child.Handle());
        DecomposeTransform(newLocal, transform.localPosition, transform.localRotation, transform.localScale);
    }

    RecalculateActiveInHierarchy(child);
    PushEvent({SceneEventType::ParentChanged, childId, {}});
}

void Scene::RemoveParent(Entity child, bool keepWorldTransform) {
    if (!IsValid(child)) {
        return;
    }
    if (!m_Registry.all_of<ParentComponent>(child.Handle())) {
        return;
    }

    const UUID childId = m_Registry.get<IDComponent>(child.Handle()).id;

    glm::mat4 oldWorld(1.0f);
    if (keepWorldTransform) {
        oldWorld = WorldMatrix(child.Handle());
    }

    RemoveFromParentChildList(child.Handle());
    m_Registry.remove<ParentComponent>(child.Handle());

    if (keepWorldTransform) {
        auto& transform = m_Registry.get<TransformComponent>(child.Handle());
        DecomposeTransform(oldWorld, transform.localPosition, transform.localRotation, transform.localScale);
    }

    RecalculateActiveInHierarchy(child);
    PushEvent({SceneEventType::ParentChanged, childId, {}});
}

std::vector<Entity> Scene::GetRootEntities() const {
    std::vector<Entity> roots;
    auto view = m_Registry.view<IDComponent>();
    for (const entt::entity handle : view) {
        if (!m_Registry.all_of<ParentComponent>(handle)) {
            roots.emplace_back(handle, const_cast<Scene*>(this));
        }
    }
    return roots;
}

std::vector<Entity> Scene::GetChildren(Entity entity) const {
    std::vector<Entity> result;
    const auto* children = m_Registry.try_get<ChildrenComponent>(entity.Handle());
    if (children == nullptr) {
        return result;
    }
    for (const UUID id : children->children) {
        const entt::entity handle = HandleFromUUID(id);
        if (handle != entt::null) {
            result.emplace_back(handle, const_cast<Scene*>(this));
        }
    }
    return result;
}

void Scene::ApplyActiveRecursive(entt::entity handle, bool parentActiveInHierarchy) {
    auto& active = m_Registry.get<ActiveComponent>(handle);
    const bool activeInHierarchy = active.active && parentActiveInHierarchy;
    active.activeInHierarchy = activeInHierarchy;

    if (const auto* children = m_Registry.try_get<ChildrenComponent>(handle)) {
        const std::vector<UUID> childIds = children->children;
        for (const UUID id : childIds) {
            const entt::entity childHandle = HandleFromUUID(id);
            if (childHandle != entt::null) {
                ApplyActiveRecursive(childHandle, activeInHierarchy);
            }
        }
    }
}

void Scene::RecalculateActiveInHierarchy(Entity entity) {
    if (!IsValid(entity)) {
        return;
    }
    const entt::entity handle = entity.Handle();
    bool parentActiveInHierarchy = true;
    if (const auto* parent = m_Registry.try_get<ParentComponent>(handle)) {
        const entt::entity parentHandle = HandleFromUUID(parent->parentId);
        if (parentHandle != entt::null) {
            parentActiveInHierarchy = m_Registry.get<ActiveComponent>(parentHandle).activeInHierarchy;
        }
    }
    ApplyActiveRecursive(handle, parentActiveInHierarchy);
}

void Scene::RecalculateAllActiveInHierarchy() {
    for (const Entity root : GetRootEntities()) {
        ApplyActiveRecursive(root.Handle(), true);
    }
}

void Scene::SetActive(Entity entity, bool active) {
    if (!IsValid(entity)) {
        return;
    }
    auto& component = m_Registry.get<ActiveComponent>(entity.Handle());
    if (component.active == active) {
        return;
    }
    component.active = active;
    RecalculateActiveInHierarchy(entity);
    PushEvent({SceneEventType::ActiveChanged, m_Registry.get<IDComponent>(entity.Handle()).id, {}});
}

} // namespace Hockey
