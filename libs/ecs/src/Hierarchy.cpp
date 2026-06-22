#include "Hockey/ECS/Hierarchy.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>

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

namespace {

void InsertUnique(std::vector<UUID>& ids, UUID id, std::size_t index) {
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    const std::size_t clamped = std::min(index, ids.size());
    ids.insert(ids.begin() + static_cast<std::ptrdiff_t>(clamped), id);
}

std::size_t IndexOf(const std::vector<UUID>& ids, UUID id) {
    const auto it = std::find(ids.begin(), ids.end(), id);
    return it == ids.end() ? ids.size() : static_cast<std::size_t>(std::distance(ids.begin(), it));
}

} // namespace

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
    RemoveFromRootOrder(childId);
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
    AppendRoot(childId);

    if (keepWorldTransform) {
        auto& transform = m_Registry.get<TransformComponent>(child.Handle());
        DecomposeTransform(oldWorld, transform.localPosition, transform.localRotation, transform.localScale);
    }

    RecalculateActiveInHierarchy(child);
    PushEvent({SceneEventType::ParentChanged, childId, {}});
}

void Scene::MoveEntity(Entity child, Entity newParent, std::size_t siblingIndex, bool keepWorldTransform) {
    if (!IsValid(child)) {
        return;
    }
    if (newParent && !IsValid(newParent)) {
        return;
    }
    if (newParent && child == newParent) {
        HK_CORE_WARN("MoveEntity: an entity cannot be parented to itself");
        return;
    }
    if (newParent && IsDescendantOf(newParent, child)) {
        HK_CORE_WARN("MoveEntity: cannot parent an entity to its own descendant");
        return;
    }

    const UUID childId = m_Registry.get<IDComponent>(child.Handle()).id;
    const UUID newParentId = newParent ? m_Registry.get<IDComponent>(newParent.Handle()).id : UUID(0);

    glm::mat4 oldWorld(1.0f);
    if (keepWorldTransform) {
        oldWorld = WorldMatrix(child.Handle());
    }

    RemoveFromParentChildList(child.Handle());
    RemoveFromRootOrder(childId);

    if (newParent) {
        m_Registry.emplace_or_replace<ParentComponent>(child.Handle(), ParentComponent{newParentId});
        auto& children = m_Registry.get<ChildrenComponent>(newParent.Handle()).children;
        InsertUnique(children, childId, siblingIndex);
    } else {
        if (m_Registry.all_of<ParentComponent>(child.Handle())) {
            m_Registry.remove<ParentComponent>(child.Handle());
        }
        InsertRoot(childId, siblingIndex);
    }

    if (keepWorldTransform) {
        auto& transform = m_Registry.get<TransformComponent>(child.Handle());
        if (newParent) {
            const glm::mat4 parentWorld = WorldMatrix(newParent.Handle());
            const glm::mat4 newLocal = glm::inverse(parentWorld) * oldWorld;
            DecomposeTransform(newLocal, transform.localPosition, transform.localRotation, transform.localScale);
        } else {
            DecomposeTransform(oldWorld, transform.localPosition, transform.localRotation, transform.localScale);
        }
    }

    RecalculateActiveInHierarchy(child);
    PushEvent({SceneEventType::ParentChanged, childId, {}});
}

void Scene::SetRootEntityOrder(const std::vector<UUID>& rootOrder) {
    m_RootOrder.clear();
    for (const UUID id : rootOrder) {
        const entt::entity handle = HandleFromUUID(id);
        if (handle != entt::null && !m_Registry.all_of<ParentComponent>(handle)) {
            AppendRoot(id);
        }
    }
    NormalizeRootOrder();
}

std::size_t Scene::GetSiblingIndex(Entity entity) const {
    if (!IsValid(entity)) {
        return 0;
    }

    const UUID id = m_Registry.get<IDComponent>(entity.Handle()).id;
    if (Entity parent = GetParent(entity)) {
        const auto* children = m_Registry.try_get<ChildrenComponent>(parent.Handle());
        return children == nullptr ? 0 : IndexOf(children->children, id);
    }

    std::vector<UUID> roots;
    roots.reserve(m_RootOrder.size());
    for (const UUID rootId : m_RootOrder) {
        const entt::entity handle = HandleFromUUID(rootId);
        if (handle != entt::null && !m_Registry.all_of<ParentComponent>(handle)) {
            roots.push_back(rootId);
        }
    }
    return IndexOf(roots, id);
}

std::vector<Entity> Scene::GetRootEntities() const {
    std::vector<Entity> roots;
    std::vector<UUID> emitted;
    emitted.reserve(m_RootOrder.size());
    for (const UUID id : m_RootOrder) {
        const entt::entity handle = HandleFromUUID(id);
        if (handle != entt::null && !m_Registry.all_of<ParentComponent>(handle)) {
            roots.emplace_back(handle, const_cast<Scene*>(this));
            emitted.push_back(id);
        }
    }

    const auto view = m_Registry.view<IDComponent>();
    for (const entt::entity handle : view) {
        if (!m_Registry.all_of<ParentComponent>(handle)) {
            const UUID id = m_Registry.get<IDComponent>(handle).id;
            if (std::find(emitted.begin(), emitted.end(), id) == emitted.end()) {
                roots.emplace_back(handle, const_cast<Scene*>(this));
            }
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
