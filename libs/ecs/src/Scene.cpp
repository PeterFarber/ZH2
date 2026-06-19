#include "Hockey/ECS/Scene.hpp"

#include <algorithm>
#include <utility>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Transform.hpp"

namespace Hockey {

namespace {

// Copies component T from one entity to another if present. Used by duplication.
template <typename T> void CopyComponent(entt::registry& registry, entt::entity from, entt::entity to) {
    if (const auto* component = registry.try_get<T>(from)) {
        registry.emplace_or_replace<T>(to, *component);
    }
}

} // namespace

Scene::Scene(std::string name)
    : m_Name(std::move(name)), m_PrefabOverrides(std::make_unique<PrefabOverrideSet>()) {}
Scene::~Scene() = default;

PrefabOverrideSet& Scene::PrefabOverrides() {
    return *m_PrefabOverrides;
}
const PrefabOverrideSet& Scene::PrefabOverrides() const {
    return *m_PrefabOverrides;
}

const std::string& Scene::GetName() const {
    return m_Name;
}
void Scene::SetName(std::string name) {
    m_Name = std::move(name);
}

SceneMode Scene::GetMode() const {
    return m_Mode;
}
void Scene::SetMode(SceneMode mode) {
    m_Mode = mode;
}

Entity Scene::MakeEntity(entt::entity handle) {
    return Entity(handle, this);
}

entt::entity Scene::HandleFromUUID(UUID id) const {
    const auto it = m_EntityMap.find(id);
    if (it == m_EntityMap.end()) {
        return entt::null;
    }
    return it->second;
}

Entity Scene::CreateEntity(const std::string& name) {
    UUID id;
    while (id.Value() == 0 || m_EntityMap.find(id) != m_EntityMap.end()) {
        id = UUID();
    }
    return CreateEntityWithUUID(id, name);
}

Entity Scene::CreateEntityWithUUID(UUID id, const std::string& name) {
    const entt::entity handle = m_Registry.create();
    m_Registry.emplace<IDComponent>(handle, id);
    m_Registry.emplace<NameComponent>(handle, name);
    m_Registry.emplace<ObjectSettingsComponent>(handle);
    m_Registry.emplace<ActiveComponent>(handle);
    m_Registry.emplace<TransformComponent>(handle);
    m_Registry.emplace<ChildrenComponent>(handle);

    m_EntityMap[id] = handle;
    PushEvent({SceneEventType::EntityCreated, id, {}});
    return Entity(handle, this);
}

entt::entity Scene::DuplicateRecursive(entt::entity sourceHandle) {
    const std::string name = m_Registry.get<NameComponent>(sourceHandle).name;

    UUID newId;
    while (m_EntityMap.find(newId) != m_EntityMap.end()) {
        newId = UUID();
    }
    const Entity copy = CreateEntityWithUUID(newId, name);
    const entt::entity newHandle = copy.Handle();

    CopyComponent<ActiveComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<ObjectSettingsComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<TransformComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<TeamComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<PlayerRoleComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<PuckComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<GoalComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<SpawnPointComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<FaceoffSpotComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<RinkComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<PlayAreaComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<CameraRigMarkerComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<CameraComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<MeshRendererComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<LightComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<EnvironmentComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<ReflectionProbeComponent>(m_Registry, sourceHandle, newHandle);
    CopyComponent<DecalComponent>(m_Registry, sourceHandle, newHandle);
    // PrefabComponent is intentionally not copied on a plain duplicate.

    if (const auto* sourceChildren = m_Registry.try_get<ChildrenComponent>(sourceHandle)) {
        const std::vector<UUID> childIds = sourceChildren->children;
        for (const UUID childId : childIds) {
            const entt::entity childHandle = HandleFromUUID(childId);
            if (childHandle == entt::null) {
                continue;
            }
            const entt::entity newChildHandle = DuplicateRecursive(childHandle);
            m_Registry.emplace_or_replace<ParentComponent>(newChildHandle, ParentComponent{newId});
            const UUID newChildId = m_Registry.get<IDComponent>(newChildHandle).id;
            m_Registry.get<ChildrenComponent>(newHandle).children.push_back(newChildId);
        }
    }

    return newHandle;
}

Entity Scene::DuplicateEntity(Entity source) {
    if (!IsValid(source)) {
        return Entity();
    }
    const entt::entity newHandle = DuplicateRecursive(source.Handle());
    Entity copy = MakeEntity(newHandle);
    RecalculateActiveInHierarchy(copy);
    return copy;
}

void Scene::DestroySingle(entt::entity handle) {
    const UUID id = m_Registry.get<IDComponent>(handle).id;
    PushEvent({SceneEventType::EntityDestroyed, id, {}});
    m_EntityMap.erase(id);
    m_Registry.destroy(handle);
}

void Scene::DestroySubtree(entt::entity handle) {
    if (const auto* children = m_Registry.try_get<ChildrenComponent>(handle)) {
        const std::vector<UUID> childIds = children->children;
        for (const UUID childId : childIds) {
            const entt::entity childHandle = HandleFromUUID(childId);
            if (childHandle != entt::null) {
                DestroySubtree(childHandle);
            }
        }
    }
    DestroySingle(handle);
}

void Scene::RemoveFromParentChildList(entt::entity handle) {
    const auto* parent = m_Registry.try_get<ParentComponent>(handle);
    if (parent == nullptr) {
        return;
    }
    const entt::entity parentHandle = HandleFromUUID(parent->parentId);
    if (parentHandle == entt::null) {
        return;
    }
    auto* children = m_Registry.try_get<ChildrenComponent>(parentHandle);
    if (children == nullptr) {
        return;
    }
    const UUID myId = m_Registry.get<IDComponent>(handle).id;
    auto& vec = children->children;
    vec.erase(std::remove(vec.begin(), vec.end(), myId), vec.end());
}

void Scene::DetachChildToRoot(entt::entity childHandle) {
    const glm::mat4 world = WorldMatrix(childHandle);
    if (m_Registry.all_of<ParentComponent>(childHandle)) {
        m_Registry.remove<ParentComponent>(childHandle);
    }
    auto& transform = m_Registry.get<TransformComponent>(childHandle);
    DecomposeTransform(world, transform.localPosition, transform.localRotation, transform.localScale);
    RecalculateActiveInHierarchy(MakeEntity(childHandle));
}

void Scene::DestroyEntity(Entity entity) {
    if (!IsValid(entity)) {
        return;
    }
    const entt::entity handle = entity.Handle();
    if (const auto* children = m_Registry.try_get<ChildrenComponent>(handle)) {
        const std::vector<UUID> childIds = children->children;
        for (const UUID childId : childIds) {
            const entt::entity childHandle = HandleFromUUID(childId);
            if (childHandle != entt::null) {
                DetachChildToRoot(childHandle);
            }
        }
    }
    RemoveFromParentChildList(handle);
    DestroySingle(handle);
}

void Scene::DestroyEntityRecursive(Entity entity) {
    if (!IsValid(entity)) {
        return;
    }
    const entt::entity handle = entity.Handle();
    RemoveFromParentChildList(handle);
    DestroySubtree(handle);
}

void Scene::Clear() {
    m_Registry.clear();
    m_EntityMap.clear();
    m_PrefabOverrides->Clear();
}

Entity Scene::FindEntityByUUID(UUID id) {
    const entt::entity handle = HandleFromUUID(id);
    if (handle == entt::null || !m_Registry.valid(handle)) {
        return Entity();
    }
    return Entity(handle, this);
}

Entity Scene::FindEntityByName(const std::string& name) {
    auto view = m_Registry.view<NameComponent>();
    for (const entt::entity handle : view) {
        if (view.get<NameComponent>(handle).name == name) {
            return Entity(handle, this);
        }
    }
    return Entity();
}

bool Scene::IsValid(Entity entity) const {
    return entity.GetScene() == this && m_Registry.valid(entity.Handle());
}

bool Scene::ContainsUUID(UUID id) const {
    return m_EntityMap.find(id) != m_EntityMap.end();
}

std::size_t Scene::EntityCount() const {
    return m_EntityMap.size();
}

std::vector<Entity> Scene::GetAllEntities() {
    std::vector<Entity> result;
    auto view = m_Registry.view<IDComponent>();
    result.reserve(view.size());
    for (const entt::entity handle : view) {
        result.emplace_back(handle, this);
    }
    return result;
}

Entity Scene::GetParent(Entity entity) const {
    const auto* parent = m_Registry.try_get<ParentComponent>(entity.Handle());
    if (parent == nullptr) {
        return Entity();
    }
    const entt::entity parentHandle = HandleFromUUID(parent->parentId);
    if (parentHandle == entt::null) {
        return Entity();
    }
    return Entity(parentHandle, const_cast<Scene*>(this));
}

void Scene::OnRuntimeStart() {
    for (auto& system : m_Systems) {
        system->OnStart(*this);
    }
}

void Scene::OnRuntimeStop() {
    for (auto& system : m_Systems) {
        system->OnStop(*this);
    }
}

void Scene::OnSimulationStart() {
    for (auto& system : m_Systems) {
        system->OnStart(*this);
    }
}

void Scene::OnSimulationStop() {
    for (auto& system : m_Systems) {
        system->OnStop(*this);
    }
}

void Scene::OnUpdate(float deltaTime) {
    for (auto& system : m_Systems) {
        system->OnUpdate(*this, deltaTime);
    }
}

void Scene::OnFixedUpdate(float fixedDeltaTime) {
    for (auto& system : m_Systems) {
        system->OnFixedUpdate(*this, fixedDeltaTime);
    }
}

void Scene::AddSystem(std::unique_ptr<System> system) {
    if (system) {
        m_Systems.push_back(std::move(system));
    }
}

void Scene::ClearSystems() {
    m_Systems.clear();
}

std::size_t Scene::SystemCount() const {
    return m_Systems.size();
}

const std::vector<SceneEvent>& Scene::GetPendingEvents() const {
    return m_PendingEvents;
}

void Scene::ClearPendingEvents() {
    m_PendingEvents.clear();
}

void Scene::PushEvent(const SceneEvent& event) {
    m_PendingEvents.push_back(event);
}

entt::registry& Scene::Registry() {
    return m_Registry;
}
const entt::registry& Scene::Registry() const {
    return m_Registry;
}

} // namespace Hockey
