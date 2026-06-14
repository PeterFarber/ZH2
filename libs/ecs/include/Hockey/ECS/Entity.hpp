#pragma once

#include <type_traits>
#include <utility>

#include <entt/entt.hpp>

#include "Hockey/Core/Assert.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneEvents.hpp"

namespace Hockey {

// Lightweight, copyable handle that pairs a runtime entt entity with its scene.
// All gameplay/editor code should go through Entity rather than the registry.
class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene) : m_Handle(handle), m_Scene(scene) {}

    bool IsValid() const;
    explicit operator bool() const {
        return IsValid();
    }

    UUID GetUUID() const;

    const std::string& GetName() const;
    void SetName(const std::string& name);

    bool IsActive() const;
    bool IsActiveInHierarchy() const;
    void SetActive(bool active);

    entt::entity Handle() const {
        return m_Handle;
    }
    Scene* GetScene() const {
        return m_Scene;
    }

    template <typename T, typename... Args> T& AddComponent(Args&&... args) {
        HK_ASSERT(m_Scene != nullptr, "AddComponent on null entity");
        HK_ASSERT(!HasComponent<T>(), "AddComponent: component already exists");
        T& component = m_Scene->Registry().emplace<T>(m_Handle, std::forward<Args>(args)...);
        m_Scene->PushEvent({SceneEventType::ComponentAdded, GetUUID(), ComponentName<T>()});
        return component;
    }

    template <typename T, typename... Args> T& AddOrReplaceComponent(Args&&... args) {
        HK_ASSERT(m_Scene != nullptr, "AddOrReplaceComponent on null entity");
        const bool existed = HasComponent<T>();
        T& component = m_Scene->Registry().emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
        if (!existed) {
            m_Scene->PushEvent({SceneEventType::ComponentAdded, GetUUID(), ComponentName<T>()});
        }
        return component;
    }

    template <typename T> T& GetComponent() {
        HK_ASSERT(HasComponent<T>(), "GetComponent: missing component");
        return m_Scene->Registry().get<T>(m_Handle);
    }

    template <typename T> const T& GetComponent() const {
        HK_ASSERT(HasComponent<T>(), "GetComponent: missing component");
        return m_Scene->Registry().get<T>(m_Handle);
    }

    template <typename T> bool HasComponent() const {
        return m_Scene != nullptr && m_Scene->Registry().all_of<T>(m_Handle);
    }

    template <typename T> void RemoveComponent() {
        if constexpr (std::is_same_v<T, IDComponent> || std::is_same_v<T, TransformComponent> ||
                      std::is_same_v<T, ChildrenComponent>) {
            HK_CORE_ERROR("RemoveComponent refused: '{}' is protected on normal entities", ComponentName<T>());
            return;
        } else {
            if (!HasComponent<T>()) {
                HK_CORE_WARN("RemoveComponent: component '{}' not present", ComponentName<T>());
                return;
            }
            const UUID id = GetUUID();
            m_Scene->Registry().remove<T>(m_Handle);
            m_Scene->PushEvent({SceneEventType::ComponentRemoved, id, ComponentName<T>()});
        }
    }

    template <typename... T> bool HasAll() const {
        return m_Scene != nullptr && m_Scene->Registry().all_of<T...>(m_Handle);
    }

    template <typename... T> bool HasAny() const {
        return m_Scene != nullptr && m_Scene->Registry().any_of<T...>(m_Handle);
    }

    bool operator==(const Entity& other) const {
        return m_Handle == other.m_Handle && m_Scene == other.m_Scene;
    }
    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

private:
    entt::entity m_Handle = entt::null;
    Scene* m_Scene = nullptr;
};

} // namespace Hockey
