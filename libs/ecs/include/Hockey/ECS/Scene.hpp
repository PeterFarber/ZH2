#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/SceneEvents.hpp"
#include "Hockey/ECS/SceneMode.hpp"
#include "Hockey/ECS/System.hpp"

namespace Hockey {

class Entity;
class PrefabOverrideSet;

class Scene {
public:
    explicit Scene(std::string name = "Untitled Scene");
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    const std::string& GetName() const;
    void SetName(std::string name);

    SceneMode GetMode() const;
    void SetMode(SceneMode mode);

    // --- entity lifecycle ---------------------------------------------------
    Entity CreateEntity(const std::string& name = "GameObject");
    Entity CreateEntityWithUUID(UUID id, const std::string& name);

    Entity DuplicateEntity(Entity source);

    void DestroyEntity(Entity entity);
    void DestroyEntityRecursive(Entity entity);

    // Removes every entity (and systems are kept). Used before loading.
    void Clear();

    // --- lookup -------------------------------------------------------------
    Entity FindEntityByUUID(UUID id);
    Entity FindEntityByName(const std::string& name);

    bool IsValid(Entity entity) const;
    bool ContainsUUID(UUID id) const;

    std::size_t EntityCount() const;
    std::vector<Entity> GetAllEntities();

    // --- hierarchy ----------------------------------------------------------
    void SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
    void RemoveParent(Entity child, bool keepWorldTransform = true);
    void MoveEntity(Entity child, Entity newParent, std::size_t siblingIndex, bool keepWorldTransform = true);
    bool IsDescendantOf(Entity child, Entity possibleParent) const;

    void SetRootEntityOrder(const std::vector<UUID>& rootOrder);
    std::size_t GetSiblingIndex(Entity entity) const;
    std::vector<Entity> GetRootEntities() const;
    std::vector<Entity> GetChildren(Entity entity) const;
    Entity GetParent(Entity entity) const;

    // --- transforms ---------------------------------------------------------
    glm::mat4 GetLocalTransform(Entity entity) const;
    glm::mat4 GetWorldTransform(Entity entity) const;
    void SetWorldTransform(Entity entity, const glm::mat4& worldTransform);

    // --- active state -------------------------------------------------------
    void SetActive(Entity entity, bool active);
    void RecalculateActiveInHierarchy(Entity entity);
    void RecalculateAllActiveInHierarchy();

    // --- lifecycle ----------------------------------------------------------
    void OnRuntimeStart();
    void OnRuntimeStop();

    void OnSimulationStart();
    void OnSimulationStop();

    void OnUpdate(float deltaTime);
    void OnFixedUpdate(float fixedDeltaTime);

    void AddSystem(std::unique_ptr<System> system);
    void ClearSystems();
    std::size_t SystemCount() const;

    // --- prefab overrides ---------------------------------------------------
    // Per-instance edits authored on top of instantiated prefabs. Owned by the
    // scene so they persist through scene save/load.
    PrefabOverrideSet& PrefabOverrides();
    const PrefabOverrideSet& PrefabOverrides() const;

    // --- events -------------------------------------------------------------
    const std::vector<SceneEvent>& GetPendingEvents() const;
    void ClearPendingEvents();
    void PushEvent(const SceneEvent& event);

    // --- raw access ---------------------------------------------------------
    entt::registry& Registry();
    const entt::registry& Registry() const;

private:
    friend class Entity;

    entt::entity HandleFromUUID(UUID id) const;
    glm::mat4 WorldMatrix(entt::entity handle) const;
    void DestroySingle(entt::entity handle);
    void DestroySubtree(entt::entity handle);
    void DetachChildToRoot(entt::entity childHandle);
    void RemoveFromParentChildList(entt::entity childHandle);
    void RemoveFromRootOrder(UUID id);
    void InsertRoot(UUID id, std::size_t index);
    void AppendRoot(UUID id);
    void NormalizeRootOrder();
    Entity MakeEntity(entt::entity handle);
    void ApplyActiveRecursive(entt::entity handle, bool parentActiveInHierarchy);
    entt::entity DuplicateRecursive(entt::entity sourceHandle, std::unordered_map<UUID, UUID>& duplicateMap);
    void RemapDuplicatedStickAttachmentReferences(entt::entity handle,
                                                  const std::unordered_map<UUID, UUID>& duplicateMap);

    std::string m_Name;
    SceneMode m_Mode = SceneMode::Edit;

    entt::registry m_Registry;
    std::unordered_map<UUID, entt::entity> m_EntityMap;
    std::vector<UUID> m_RootOrder;

    std::unique_ptr<PrefabOverrideSet> m_PrefabOverrides;

    std::vector<std::unique_ptr<System>> m_Systems;
    std::vector<SceneEvent> m_PendingEvents;
};

} // namespace Hockey
