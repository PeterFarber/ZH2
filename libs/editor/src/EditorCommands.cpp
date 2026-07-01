#include "Hockey/Editor/EditorCommands.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/UndoRedo.hpp"

namespace Hockey {

namespace {

UUID MakeUniqueId(Scene& scene) {
    UUID id;
    while (id.Value() == 0 || scene.ContainsUUID(id)) {
        id = UUID();
    }
    return id;
}

// Adds 'childId' to a parent's child list if missing (used when relinking a
// restored subtree to an outer parent that survived the delete).
void LinkChildToParent(Scene& scene, UUID parentId, UUID childId) {
    Entity parent = scene.FindEntityByUUID(parentId);
    if (!parent) {
        return;
    }
    auto& children = parent.GetComponent<ChildrenComponent>().children;
    if (std::find(children.begin(), children.end(), childId) == children.end()) {
        children.push_back(childId);
    }
    Entity child = scene.FindEntityByUUID(childId);
    if (child) {
        scene.Registry().emplace_or_replace<ParentComponent>(child.Handle(), ParentComponent{parentId});
    }
}

UUID ParentIdOf(Scene& scene, const Entity& entity) {
    if (Entity parent = scene.GetParent(entity)) {
        return parent.GetUUID();
    }
    return UUID(0);
}

bool ContainsId(const std::vector<UUID>& ids, UUID id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

bool HasSelectedAncestor(Scene& scene, const Entity& entity, const std::vector<UUID>& ids) {
    for (Entity parent = scene.GetParent(entity); parent; parent = scene.GetParent(parent)) {
        if (ContainsId(ids, parent.GetUUID())) {
            return true;
        }
    }
    return false;
}

void CollectTopLevelIdsInHierarchyOrder(Scene& scene, const Entity& entity,
                                        const std::unordered_set<std::uint64_t>& selectedIds,
                                        bool ancestorSelected, std::vector<UUID>& out) {
    if (!entity) {
        return;
    }

    const UUID id = entity.GetUUID();
    const bool selected = selectedIds.contains(id.Value());
    if (selected && !ancestorSelected) {
        out.push_back(id);
    }

    const bool blockDescendants = ancestorSelected || selected;
    for (const Entity& child : scene.GetChildren(entity)) {
        CollectTopLevelIdsInHierarchyOrder(scene, child, selectedIds, blockDescendants, out);
    }
}

std::vector<UUID> NormalizeTopLevelEntityIds(Scene& scene, const std::vector<UUID>& entityIds) {
    std::unordered_set<std::uint64_t> selectedIds;
    selectedIds.reserve(entityIds.size());
    for (const UUID id : entityIds) {
        if (id.IsValid() && scene.ContainsUUID(id)) {
            selectedIds.insert(id.Value());
        }
    }

    std::vector<UUID> ordered;
    ordered.reserve(selectedIds.size());
    for (const Entity& root : scene.GetRootEntities()) {
        CollectTopLevelIdsInHierarchyOrder(scene, root, selectedIds, /*ancestorSelected=*/false, ordered);
    }
    return ordered;
}

std::vector<UUID> SiblingIdsForParent(Scene& scene, UUID parentId) {
    std::vector<UUID> ids;
    if (parentId.IsValid()) {
        Entity parent = scene.FindEntityByUUID(parentId);
        if (!parent) {
            return ids;
        }
        for (const Entity& child : scene.GetChildren(parent)) {
            ids.push_back(child.GetUUID());
        }
        return ids;
    }

    for (const Entity& root : scene.GetRootEntities()) {
        ids.push_back(root.GetUUID());
    }
    return ids;
}

std::size_t AdjustSiblingIndexForMovingIds(Scene& scene, const std::vector<UUID>& entityIds, UUID newParentId,
                                           std::size_t siblingIndex) {
    std::size_t adjusted = siblingIndex;
    for (const UUID id : entityIds) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || ParentIdOf(scene, entity) != newParentId) {
            continue;
        }
        const std::size_t oldIndex = scene.GetSiblingIndex(entity);
        if (oldIndex < adjusted) {
            --adjusted;
        }
    }
    return adjusted;
}

std::vector<UUID> BuildTargetSiblingOrder(Scene& scene, const std::vector<UUID>& entityIds, UUID newParentId,
                                          std::size_t siblingIndex) {
    std::vector<UUID> order = SiblingIdsForParent(scene, newParentId);
    order.erase(std::remove_if(order.begin(), order.end(),
                               [&entityIds](UUID id) { return ContainsId(entityIds, id); }),
                order.end());

    const std::size_t insertIndex = std::min(siblingIndex, order.size());
    order.insert(order.begin() + static_cast<std::ptrdiff_t>(insertIndex), entityIds.begin(), entityIds.end());
    return order;
}

std::vector<UUID> NormalizeCreateEmptyParentIds(Scene& scene, const std::vector<UUID>& entityIds, UUID* parentId,
                                                std::size_t* insertIndex) {
    std::vector<UUID> uniqueIds;
    uniqueIds.reserve(entityIds.size());
    for (const UUID id : entityIds) {
        if (!id.IsValid() || ContainsId(uniqueIds, id)) {
            continue;
        }
        if (!scene.ContainsUUID(id)) {
            return {};
        }
        uniqueIds.push_back(id);
    }
    if (uniqueIds.empty()) {
        return {};
    }

    Entity first = scene.FindEntityByUUID(uniqueIds.front());
    if (!first) {
        return {};
    }
    const UUID sharedParent = ParentIdOf(scene, first);
    std::size_t minIndex = scene.GetSiblingIndex(first);

    for (const UUID id : uniqueIds) {
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity || ParentIdOf(scene, entity) != sharedParent || HasSelectedAncestor(scene, entity, uniqueIds)) {
            return {};
        }
        minIndex = std::min(minIndex, scene.GetSiblingIndex(entity));
    }

    std::sort(uniqueIds.begin(), uniqueIds.end(), [&](UUID lhs, UUID rhs) {
        return scene.GetSiblingIndex(scene.FindEntityByUUID(lhs)) < scene.GetSiblingIndex(scene.FindEntityByUUID(rhs));
    });

    if (parentId != nullptr) {
        *parentId = sharedParent;
    }
    if (insertIndex != nullptr) {
        *insertIndex = minIndex;
    }
    return uniqueIds;
}

} // namespace

namespace EntitySnapshot {

std::string CaptureEntity(Scene& scene, UUID id) {
    Entity entity = scene.FindEntityByUUID(id);
    if (!entity) {
        return {};
    }
    YAML::Emitter out;
    ComponentSerializer::SerializeEntity(out, entity);
    return out.c_str();
}

std::string CaptureSubtree(Scene& scene, UUID rootId) {
    Entity root = scene.FindEntityByUUID(rootId);
    if (!root) {
        return {};
    }
    YAML::Emitter out;
    out << YAML::BeginSeq;
    std::function<void(Entity)> emit = [&](Entity entity) {
        ComponentSerializer::SerializeEntity(out, entity);
        for (const Entity& child : scene.GetChildren(entity)) {
            emit(child);
        }
    };
    emit(root);
    out << YAML::EndSeq;
    return out.c_str();
}

void ApplyEntity(Scene& scene, const std::string& yaml) {
    if (yaml.empty()) {
        return;
    }
    try {
        const YAML::Node node = YAML::Load(yaml);
        if (!node["Entity"]) {
            return;
        }
        const UUID id(node["Entity"].as<std::uint64_t>());
        Entity entity = scene.FindEntityByUUID(id);
        if (!entity) {
            return;
        }
        ComponentSerializer::DeserializeCoreComponents(entity, node);
        ComponentSerializer::DeserializeHockeyMarkerComponents(entity, node);
        ComponentSerializer::DeserializeRenderComponents(entity, node);
        ComponentSerializer::DeserializeExternalComponents(entity, node);
        scene.RecalculateActiveInHierarchy(entity);
    } catch (const std::exception& e) {
        HK_EDITOR_ERROR("ApplyEntity snapshot parse failed: {}", e.what());
    }
}

UUID RestoreSubtree(Scene& scene, const std::string& yaml, UUID parentId) {
    if (yaml.empty()) {
        return UUID(0);
    }
    UUID rootId(0);
    try {
        const YAML::Node seq = YAML::Load(yaml);
        if (!seq.IsSequence() || seq.size() == 0) {
            return UUID(0);
        }
        for (const auto entityNode : seq) {
            Entity created = ComponentSerializer::DeserializeEntity(scene, entityNode);
            if (!rootId.IsValid() && created) {
                rootId = created.GetUUID();
            }
        }
    } catch (const std::exception& e) {
        HK_EDITOR_ERROR("RestoreSubtree snapshot parse failed: {}", e.what());
        return UUID(0);
    }

    if (rootId.IsValid() && parentId.IsValid() && scene.ContainsUUID(parentId)) {
        LinkChildToParent(scene, parentId, rootId);
    } else if (rootId.IsValid()) {
        // Restored at scene root: drop any stale parent reference.
        if (Entity root = scene.FindEntityByUUID(rootId)) {
            if (root.HasComponent<ParentComponent>()) {
                scene.Registry().remove<ParentComponent>(root.Handle());
            }
        }
    }
    scene.RecalculateAllActiveInHierarchy();
    return rootId;
}

UUID InstantiateSubtree(Scene& scene, const std::string& yaml, UUID parentId) {
    if (yaml.empty()) {
        return UUID(0);
    }
    UUID newRootId(0);
    try {
        YAML::Node seq = YAML::Load(yaml);
        if (!seq.IsSequence() || seq.size() == 0) {
            return UUID(0);
        }

        // Map every captured id to a fresh, scene-unique id.
        std::unordered_map<std::uint64_t, std::uint64_t> remap;
        for (const auto entityNode : seq) {
            const std::uint64_t oldId = entityNode["Entity"].as<std::uint64_t>();
            remap[oldId] = MakeUniqueId(scene).Value();
        }

        const std::uint64_t oldRootId = seq[0]["Entity"].as<std::uint64_t>();
        newRootId = UUID(remap[oldRootId]);

        for (auto entityNode : seq) {
            const std::uint64_t oldId = entityNode["Entity"].as<std::uint64_t>();
            entityNode["Entity"] = remap[oldId];

            if (auto parentNode = entityNode["ParentComponent"]; parentNode && parentNode["Parent"]) {
                const std::uint64_t oldParent = parentNode["Parent"].as<std::uint64_t>();
                const auto it = remap.find(oldParent);
                if (it != remap.end()) {
                    parentNode["Parent"] = it->second;
                } else {
                    // Parent lived outside the captured subtree: become a root and
                    // get re-parented explicitly below.
                    entityNode.remove("ParentComponent");
                }
            }

            if (auto childrenNode = entityNode["ChildrenComponent"]; childrenNode && childrenNode["Children"]) {
                YAML::Node rebuilt(YAML::NodeType::Sequence);
                for (const auto child : childrenNode["Children"]) {
                    const std::uint64_t oldChild = child.as<std::uint64_t>();
                    const auto it = remap.find(oldChild);
                    if (it != remap.end()) {
                        rebuilt.push_back(it->second);
                    }
                }
                childrenNode["Children"] = rebuilt;
            }

            ComponentSerializer::DeserializeEntity(scene, entityNode);
        }
    } catch (const std::exception& e) {
        HK_EDITOR_ERROR("InstantiateSubtree snapshot parse failed: {}", e.what());
        return UUID(0);
    }

    if (newRootId.IsValid()) {
        Entity root = scene.FindEntityByUUID(newRootId);
        if (root) {
            if (parentId.IsValid() && scene.ContainsUUID(parentId)) {
                Entity parent = scene.FindEntityByUUID(parentId);
                if (parent && !scene.IsDescendantOf(parent, root)) {
                    scene.SetParent(root, parent, /*keepWorldTransform=*/false);
                }
            } else if (root.HasComponent<ParentComponent>()) {
                scene.Registry().remove<ParentComponent>(root.Handle());
            }
        }
    }
    scene.RecalculateAllActiveInHierarchy();
    return newRootId;
}

} // namespace EntitySnapshot

namespace {

// ----- Concrete commands -------------------------------------------------

class CreateEntityCommand : public EditorCommand {
public:
    CreateEntityCommand(std::string name, UUID parentId) : m_Name(std::move(name)), m_ParentId(parentId) {}

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (!m_Id.IsValid()) {
            m_Id = MakeUniqueId(*scene);
        }
        Entity entity = scene->CreateEntityWithUUID(m_Id, m_Name);
        if (m_ParentId.IsValid()) {
            if (Entity parent = scene->FindEntityByUUID(m_ParentId)) {
                scene->SetParent(entity, parent);
            }
        }
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (Entity entity = scene->FindEntityByUUID(m_Id)) {
            scene->DestroyEntityRecursive(entity);
        }
        context.selection.Remove(m_Id);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Create Entity";
    }

private:
    std::string m_Name;
    UUID m_ParentId;
    UUID m_Id{0};
};

class DeleteEntityCommand : public EditorCommand {
public:
    DeleteEntityCommand(Scene& scene, UUID entityId) : m_Id(entityId) {
        m_Snapshot = EntitySnapshot::CaptureSubtree(scene, entityId);
        if (Entity entity = scene.FindEntityByUUID(entityId)) {
            if (Entity parent = scene.GetParent(entity)) {
                m_ParentId = parent.GetUUID();
            }
        }
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (Entity entity = scene->FindEntityByUUID(m_Id)) {
            scene->DestroyEntityRecursive(entity);
        }
        context.selection.Remove(m_Id);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        const UUID root = EntitySnapshot::RestoreSubtree(*scene, m_Snapshot, m_ParentId);
        if (root.IsValid()) {
            context.selection.Select(root);
        }
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Delete Entity";
    }

private:
    UUID m_Id;
    UUID m_ParentId{0};
    std::string m_Snapshot;
};

class DuplicateEntityCommand : public EditorCommand {
public:
    explicit DuplicateEntityCommand(UUID sourceId) : m_SourceId(sourceId) {}

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (!m_Captured) {
            Entity source = scene->FindEntityByUUID(m_SourceId);
            if (!source) {
                return;
            }
            if (Entity parent = scene->GetParent(source)) {
                m_ParentId = parent.GetUUID();
            }
            m_SiblingIndex = scene->GetSiblingIndex(source) + 1;

            Entity copy = scene->DuplicateEntity(source);
            if (!copy) {
                return;
            }
            m_NewId = copy.GetUUID();
            MoveDuplicateToCapturedSlot(*scene, copy);
            m_Snapshot = EntitySnapshot::CaptureSubtree(*scene, m_NewId);
            m_Captured = true;
        } else {
            m_NewId = EntitySnapshot::RestoreSubtree(*scene, m_Snapshot, m_ParentId);
            if (Entity copy = scene->FindEntityByUUID(m_NewId)) {
                MoveDuplicateToCapturedSlot(*scene, copy);
            }
        }
        if (m_NewId.IsValid()) {
            context.selection.Select(m_NewId);
        }
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (Entity entity = scene->FindEntityByUUID(m_NewId)) {
            scene->DestroyEntityRecursive(entity);
        }
        context.selection.Remove(m_NewId);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Duplicate Entity";
    }

private:
    void MoveDuplicateToCapturedSlot(Scene& scene, Entity copy) const {
        if (!copy) {
            return;
        }

        Entity parent;
        if (m_ParentId.IsValid()) {
            parent = scene.FindEntityByUUID(m_ParentId);
            if (!parent || copy == parent) {
                return;
            }
        }

        scene.MoveEntity(copy, parent, m_SiblingIndex, /*keepWorldTransform=*/false);
    }

    UUID m_SourceId;
    UUID m_NewId{0};
    UUID m_ParentId{0};
    std::size_t m_SiblingIndex = 0;
    std::string m_Snapshot;
    bool m_Captured = false;
};

class CreateEmptyParentCommand : public EditorCommand {
public:
    CreateEmptyParentCommand(Scene& scene, std::vector<UUID> entityIds) {
        m_ChildIds = NormalizeCreateEmptyParentIds(scene, entityIds, &m_ParentId, &m_InsertIndex);
        m_ChildStates.reserve(m_ChildIds.size());
        for (const UUID id : m_ChildIds) {
            Entity entity = scene.FindEntityByUUID(id);
            if (!entity) {
                continue;
            }
            const auto& transform = entity.GetComponent<TransformComponent>();
            m_ChildStates.push_back({id,
                                     scene.GetSiblingIndex(entity),
                                     {transform.localPosition, transform.localRotation, transform.localScale}});
        }
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || m_ChildStates.empty()) {
            return;
        }
        if (!m_WrapperId.IsValid()) {
            m_WrapperId = MakeUniqueId(*scene);
        }

        Entity wrapper = scene->FindEntityByUUID(m_WrapperId);
        if (!wrapper) {
            wrapper = scene->CreateEntityWithUUID(m_WrapperId, "GameObject");
        }
        Entity parent;
        if (m_ParentId.IsValid()) {
            parent = scene->FindEntityByUUID(m_ParentId);
            if (!parent) {
                return;
            }
        }
        scene->MoveEntity(wrapper, parent, m_InsertIndex, /*keepWorldTransform=*/false);

        for (std::size_t i = 0; i < m_ChildStates.size(); ++i) {
            Entity child = scene->FindEntityByUUID(m_ChildStates[i].id);
            if (child && child != wrapper && !scene->IsDescendantOf(wrapper, child)) {
                scene->MoveEntity(child, wrapper, i, /*keepWorldTransform=*/true);
            }
        }

        context.selection.Select(m_WrapperId);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || !m_WrapperId.IsValid()) {
            return;
        }

        Entity parent;
        if (m_ParentId.IsValid()) {
            parent = scene->FindEntityByUUID(m_ParentId);
        }
        for (const WrappedChildState& state : m_ChildStates) {
            Entity child = scene->FindEntityByUUID(state.id);
            if (!child) {
                continue;
            }
            scene->MoveEntity(child, parent, state.siblingIndex, /*keepWorldTransform=*/false);
            if (child.HasComponent<TransformComponent>()) {
                auto& transform = child.GetComponent<TransformComponent>();
                transform.localPosition = state.local.position;
                transform.localRotation = state.local.rotation;
                transform.localScale = state.local.scale;
                scene->RecalculateActiveInHierarchy(child);
            }
        }

        if (Entity wrapper = scene->FindEntityByUUID(m_WrapperId)) {
            scene->DestroyEntityRecursive(wrapper);
        }
        context.selection.Clear();
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Create Empty Parent";
    }

private:
    struct WrappedChildState {
        UUID id;
        std::size_t siblingIndex = 0;
        TransformData local;
    };

    std::vector<UUID> m_ChildIds;
    std::vector<WrappedChildState> m_ChildStates;
    UUID m_ParentId{0};
    std::size_t m_InsertIndex = 0;
    UUID m_WrapperId{0};
};

// Generic "spawn" command: a builder creates one or more root entities on first
// execution; their subtrees are snapshotted so undo can delete them and redo can
// restore them with the original UUIDs/components.
class SpawnEntitiesCommand : public EditorCommand {
public:
    SpawnEntitiesCommand(std::string name, EditorCommands::SpawnBuilder builder)
        : m_Name(std::move(name)), m_Builder(std::move(builder)) {}

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (!m_Captured) {
            m_Roots = m_Builder ? m_Builder(*scene) : std::vector<UUID>{};
            m_Snapshots.clear();
            m_Snapshots.reserve(m_Roots.size());
            for (const UUID root : m_Roots) {
                m_Snapshots.push_back(EntitySnapshot::CaptureSubtree(*scene, root));
            }
            m_Captured = true;
        } else {
            for (std::size_t i = 0; i < m_Snapshots.size(); ++i) {
                m_Roots[i] = EntitySnapshot::RestoreSubtree(*scene, m_Snapshots[i], UUID(0));
            }
        }

        context.selection.Clear();
        bool first = true;
        for (const UUID root : m_Roots) {
            if (!root.IsValid()) {
                continue;
            }
            if (first) {
                context.selection.Select(root);
                first = false;
            } else {
                context.selection.Add(root);
            }
        }
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        for (const UUID root : m_Roots) {
            if (Entity entity = scene->FindEntityByUUID(root)) {
                scene->DestroyEntityRecursive(entity);
            }
            context.selection.Remove(root);
        }
        context.MarkDirty();
    }

    std::string Name() const override {
        return m_Name;
    }

private:
    std::string m_Name;
    EditorCommands::SpawnBuilder m_Builder;
    std::vector<UUID> m_Roots;
    std::vector<std::string> m_Snapshots;
    bool m_Captured = false;
};

class RenameEntityCommand : public EditorCommand {
public:
    RenameEntityCommand(UUID entityId, std::string oldName, std::string newName)
        : m_Id(entityId), m_OldName(std::move(oldName)), m_NewName(std::move(newName)) {}

    void Execute(EditorContext& context) override {
        Apply(context, m_NewName);
    }
    void Undo(EditorContext& context) override {
        Apply(context, m_OldName);
    }
    std::string Name() const override {
        return "Rename Entity";
    }

private:
    void Apply(EditorContext& context, const std::string& name) {
        if (context.activeScene == nullptr) {
            return;
        }
        if (Entity entity = context.activeScene->FindEntityByUUID(m_Id)) {
            entity.SetName(name);
            context.MarkDirty();
        }
    }

    UUID m_Id;
    std::string m_OldName;
    std::string m_NewName;
};

class SetActiveCommand : public EditorCommand {
public:
    SetActiveCommand(UUID entityId, bool oldValue, bool newValue)
        : m_Id(entityId), m_OldValue(oldValue), m_NewValue(newValue) {}

    void Execute(EditorContext& context) override {
        Apply(context, m_NewValue);
    }
    void Undo(EditorContext& context) override {
        Apply(context, m_OldValue);
    }
    std::string Name() const override {
        return "Set Active";
    }

private:
    void Apply(EditorContext& context, bool value) {
        if (context.activeScene == nullptr) {
            return;
        }
        if (Entity entity = context.activeScene->FindEntityByUUID(m_Id)) {
            context.activeScene->SetActive(entity, value);
            context.MarkDirty();
        }
    }

    UUID m_Id;
    bool m_OldValue;
    bool m_NewValue;
};

class TransformEntityCommand : public EditorCommand {
public:
    TransformEntityCommand(UUID entityId, const TransformData& oldValue, const TransformData& newValue)
        : m_Id(entityId), m_Old(oldValue), m_New(newValue) {}

    void Execute(EditorContext& context) override {
        Apply(context, m_New);
    }
    void Undo(EditorContext& context) override {
        Apply(context, m_Old);
    }
    std::string Name() const override {
        return "Transform Entity";
    }

private:
    void Apply(EditorContext& context, const TransformData& value) {
        if (context.activeScene == nullptr) {
            return;
        }
        Entity entity = context.activeScene->FindEntityByUUID(m_Id);
        if (!entity) {
            return;
        }
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.localPosition = value.position;
        transform.localRotation = value.rotation;
        transform.localScale = value.scale;
        context.MarkDirty();
    }

    UUID m_Id;
    TransformData m_Old;
    TransformData m_New;
};

class TransformEntitiesCommand : public EditorCommand {
public:
    explicit TransformEntitiesCommand(std::vector<EntityTransformSnapshot> snapshots)
        : m_Snapshots(std::move(snapshots)) {}

    void Execute(EditorContext& context) override {
        Apply(context, /*useAfter=*/true);
    }
    void Undo(EditorContext& context) override {
        Apply(context, /*useAfter=*/false);
    }
    std::string Name() const override {
        return "Transform Entities";
    }

private:
    void Apply(EditorContext& context, bool useAfter) {
        if (context.activeScene == nullptr) {
            return;
        }

        bool applied = false;
        for (const EntityTransformSnapshot& snapshot : m_Snapshots) {
            Entity entity = context.activeScene->FindEntityByUUID(snapshot.entityId);
            if (!entity || !entity.HasComponent<TransformComponent>()) {
                continue;
            }

            const TransformData& value = useAfter ? snapshot.after : snapshot.before;
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.localPosition = value.position;
            transform.localRotation = value.rotation;
            transform.localScale = value.scale;
            applied = true;
        }

        if (applied) {
            context.MarkDirty();
        }
    }

    std::vector<EntityTransformSnapshot> m_Snapshots;
};

class SetParentCommand : public EditorCommand {
public:
    SetParentCommand(Scene& scene, UUID childId, UUID newParentId) : m_ChildId(childId), m_NewParentId(newParentId) {
        if (Entity child = scene.FindEntityByUUID(childId)) {
            if (Entity parent = scene.GetParent(child)) {
                m_OldParentId = parent.GetUUID();
            }
            const auto& transform = child.GetComponent<TransformComponent>();
            m_OldLocal = {transform.localPosition, transform.localRotation, transform.localScale};
        }
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        Entity child = scene->FindEntityByUUID(m_ChildId);
        if (!child) {
            return;
        }
        if (m_NewParentId.IsValid()) {
            Entity parent = scene->FindEntityByUUID(m_NewParentId);
            if (parent && child != parent && !scene->IsDescendantOf(parent, child)) {
                scene->SetParent(child, parent);
            }
        } else {
            scene->RemoveParent(child);
        }
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        Entity child = scene->FindEntityByUUID(m_ChildId);
        if (!child) {
            return;
        }
        if (m_OldParentId.IsValid()) {
            if (Entity parent = scene->FindEntityByUUID(m_OldParentId)) {
                scene->SetParent(child, parent, /*keepWorldTransform=*/false);
            }
        } else {
            scene->RemoveParent(child, /*keepWorldTransform=*/false);
        }
        auto& transform = child.GetComponent<TransformComponent>();
        transform.localPosition = m_OldLocal.position;
        transform.localRotation = m_OldLocal.rotation;
        transform.localScale = m_OldLocal.scale;
        scene->RecalculateActiveInHierarchy(child);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Set Parent";
    }

private:
    UUID m_ChildId;
    UUID m_NewParentId;
    UUID m_OldParentId{0};
    TransformData m_OldLocal;
};

class MoveEntityCommand : public EditorCommand {
public:
    MoveEntityCommand(Scene& scene, UUID childId, UUID newParentId, std::size_t siblingIndex)
        : m_ChildId(childId), m_NewParentId(newParentId), m_NewIndex(siblingIndex) {
        if (Entity child = scene.FindEntityByUUID(childId)) {
            if (Entity parent = scene.GetParent(child)) {
                m_OldParentId = parent.GetUUID();
            }
            m_OldIndex = scene.GetSiblingIndex(child);
            const auto& transform = child.GetComponent<TransformComponent>();
            m_OldLocal = {transform.localPosition, transform.localRotation, transform.localScale};
        }
    }

    void Execute(EditorContext& context) override {
        Move(context, m_NewParentId, m_NewIndex, /*keepWorldTransform=*/true, std::nullopt);
    }

    void Undo(EditorContext& context) override {
        Move(context, m_OldParentId, m_OldIndex, /*keepWorldTransform=*/false, m_OldLocal);
    }

    std::string Name() const override {
        return "Move Entity";
    }

private:
    void Move(EditorContext& context, UUID parentId, std::size_t siblingIndex, bool keepWorldTransform,
              std::optional<TransformData> restoreLocal) {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }

        Entity child = scene->FindEntityByUUID(m_ChildId);
        if (!child) {
            return;
        }

        Entity parent;
        if (parentId.IsValid()) {
            parent = scene->FindEntityByUUID(parentId);
            if (!parent || child == parent || scene->IsDescendantOf(parent, child)) {
                return;
            }
        }

        scene->MoveEntity(child, parent, siblingIndex, keepWorldTransform);

        if (restoreLocal.has_value() && child.HasComponent<TransformComponent>()) {
            auto& transform = child.GetComponent<TransformComponent>();
            transform.localPosition = restoreLocal->position;
            transform.localRotation = restoreLocal->rotation;
            transform.localScale = restoreLocal->scale;
            scene->RecalculateActiveInHierarchy(child);
        }

        context.MarkDirty();
    }

    UUID m_ChildId;
    UUID m_NewParentId;
    std::size_t m_NewIndex = 0;
    UUID m_OldParentId{0};
    std::size_t m_OldIndex = 0;
    TransformData m_OldLocal;
};

class MoveEntitiesCommand : public EditorCommand {
public:
    MoveEntitiesCommand(Scene& scene, std::vector<UUID> entityIds, UUID newParentId, std::size_t siblingIndex)
        : m_EntityIds(NormalizeTopLevelEntityIds(scene, entityIds)), m_NewParentId(newParentId) {
        m_NewIndex = AdjustSiblingIndexForMovingIds(scene, m_EntityIds, newParentId, siblingIndex);
        m_TargetOrder = BuildTargetSiblingOrder(scene, m_EntityIds, newParentId, m_NewIndex);

        m_OldStates.reserve(m_EntityIds.size());
        for (const UUID id : m_EntityIds) {
            Entity entity = scene.FindEntityByUUID(id);
            if (!entity) {
                continue;
            }
            EntityMoveState state;
            state.id = id;
            state.parentId = ParentIdOf(scene, entity);
            state.siblingIndex = scene.GetSiblingIndex(entity);
            if (entity.HasComponent<TransformComponent>()) {
                const auto& transform = entity.GetComponent<TransformComponent>();
                state.local = {transform.localPosition, transform.localRotation, transform.localScale};
            }
            m_OldStates.push_back(state);
        }
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || m_EntityIds.empty() || m_TargetOrder.empty()) {
            return;
        }

        Entity parent;
        if (m_NewParentId.IsValid()) {
            parent = scene->FindEntityByUUID(m_NewParentId);
            if (!parent) {
                return;
            }
        }

        for (const UUID id : m_EntityIds) {
            Entity child = scene->FindEntityByUUID(id);
            if (!child) {
                continue;
            }
            if (parent && (child == parent || scene->IsDescendantOf(parent, child))) {
                return;
            }
        }

        bool movedAny = false;
        for (std::size_t index = 0; index < m_TargetOrder.size(); ++index) {
            Entity entity = scene->FindEntityByUUID(m_TargetOrder[index]);
            if (!entity) {
                continue;
            }
            scene->MoveEntity(entity, parent, index, /*keepWorldTransform=*/true);
            movedAny = true;
        }

        if (movedAny) {
            SelectMovedEntities(context);
            context.MarkDirty();
        }
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || m_OldStates.empty()) {
            return;
        }

        bool restoredAny = false;
        for (const EntityMoveState& state : m_OldStates) {
            Entity entity = scene->FindEntityByUUID(state.id);
            if (!entity) {
                continue;
            }

            Entity parent;
            if (state.parentId.IsValid()) {
                parent = scene->FindEntityByUUID(state.parentId);
                if (!parent) {
                    continue;
                }
            }

            scene->MoveEntity(entity, parent, state.siblingIndex, /*keepWorldTransform=*/false);
            if (entity.HasComponent<TransformComponent>()) {
                auto& transform = entity.GetComponent<TransformComponent>();
                transform.localPosition = state.local.position;
                transform.localRotation = state.local.rotation;
                transform.localScale = state.local.scale;
                scene->RecalculateActiveInHierarchy(entity);
            }
            restoredAny = true;
        }

        if (restoredAny) {
            SelectMovedEntities(context);
            context.MarkDirty();
        }
    }

    std::string Name() const override {
        return m_EntityIds.size() > 1 ? "Move Entities" : "Move Entity";
    }

private:
    struct EntityMoveState {
        UUID id{0};
        UUID parentId{0};
        std::size_t siblingIndex = 0;
        TransformData local;
    };

    void SelectMovedEntities(EditorContext& context) {
        if (context.activeScene == nullptr) {
            return;
        }
        context.selection.Clear();
        for (const UUID id : m_EntityIds) {
            if (context.activeScene->ContainsUUID(id)) {
                context.selection.Add(id);
            }
        }
    }

    std::vector<UUID> m_EntityIds;
    UUID m_NewParentId{0};
    std::size_t m_NewIndex = 0;
    std::vector<UUID> m_TargetOrder;
    std::vector<EntityMoveState> m_OldStates;
};

class AddComponentCommand : public EditorCommand {
public:
    AddComponentCommand(UUID entityId, std::string componentName)
        : m_Id(entityId), m_Component(std::move(componentName)) {}

    void Execute(EditorContext& context) override {
        if (context.activeScene == nullptr) {
            return;
        }
        const ComponentMetadata* metadata = ComponentRegistry::Get().FindByName(m_Component);
        if (metadata == nullptr || !metadata->add) {
            return;
        }
        Entity entity = context.activeScene->FindEntityByUUID(m_Id);
        if (!entity || (metadata->has && metadata->has(entity))) {
            return;
        }
        metadata->add(entity);
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        if (context.activeScene == nullptr) {
            return;
        }
        const ComponentMetadata* metadata = ComponentRegistry::Get().FindByName(m_Component);
        if (metadata == nullptr || !metadata->remove) {
            return;
        }
        if (Entity entity = context.activeScene->FindEntityByUUID(m_Id)) {
            metadata->remove(entity);
        }
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Add Component";
    }

private:
    UUID m_Id;
    std::string m_Component;
};

class RemoveComponentCommand : public EditorCommand {
public:
    RemoveComponentCommand(Scene& scene, UUID entityId, std::string componentName)
        : m_Id(entityId), m_Component(std::move(componentName)) {
        m_Before = EntitySnapshot::CaptureEntity(scene, entityId);
    }

    void Execute(EditorContext& context) override {
        if (context.activeScene == nullptr) {
            return;
        }
        const ComponentMetadata* metadata = ComponentRegistry::Get().FindByName(m_Component);
        if (metadata == nullptr || !metadata->remove) {
            return;
        }
        if (Entity entity = context.activeScene->FindEntityByUUID(m_Id)) {
            metadata->remove(entity);
        }
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        if (context.activeScene == nullptr) {
            return;
        }
        EntitySnapshot::ApplyEntity(*context.activeScene, m_Before);
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Remove Component";
    }

private:
    UUID m_Id;
    std::string m_Component;
    std::string m_Before;
};

class EditComponentFieldCommand : public EditorCommand {
public:
    EditComponentFieldCommand(UUID entityId, std::string label, std::string beforeYaml, std::string afterYaml)
        : m_Id(entityId), m_Label(std::move(label)), m_Before(std::move(beforeYaml)), m_After(std::move(afterYaml)) {}

    void Execute(EditorContext& context) override {
        Apply(context, m_After);
    }
    void Undo(EditorContext& context) override {
        Apply(context, m_Before);
    }
    std::string Name() const override {
        return m_Label.empty() ? "Edit Field" : ("Edit " + m_Label);
    }

private:
    void Apply(EditorContext& context, const std::string& yaml) {
        if (context.activeScene == nullptr) {
            return;
        }
        EntitySnapshot::ApplyEntity(*context.activeScene, yaml);
        if (m_Id.IsValid()) {
            context.selection.Select(m_Id);
        }
        context.MarkDirty();
    }

    UUID m_Id;
    std::string m_Label;
    std::string m_Before;
    std::string m_After;
};

class PasteEntityCommand : public EditorCommand {
public:
    PasteEntityCommand(std::string subtreeYaml, UUID parentId)
        : m_Source(std::move(subtreeYaml)), m_ParentId(parentId) {}

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (!m_Created) {
            m_NewId = EntitySnapshot::InstantiateSubtree(*scene, m_Source, m_ParentId);
            if (m_NewId.IsValid()) {
                // Capture the materialised subtree so redo recreates identical ids.
                m_Redo = EntitySnapshot::CaptureSubtree(*scene, m_NewId);
                m_Created = true;
            }
        } else {
            m_NewId = EntitySnapshot::RestoreSubtree(*scene, m_Redo, m_ParentId);
        }
        if (m_NewId.IsValid()) {
            context.selection.Select(m_NewId);
        }
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (Entity entity = scene->FindEntityByUUID(m_NewId)) {
            scene->DestroyEntityRecursive(entity);
        }
        context.selection.Remove(m_NewId);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Paste Entity";
    }

private:
    std::string m_Source;
    std::string m_Redo;
    UUID m_ParentId{0};
    UUID m_NewId{0};
    bool m_Created = false;
};

class PasteComponentCommand : public EditorCommand {
public:
    PasteComponentCommand(Scene& scene, UUID targetId, std::string componentName, std::string componentYaml)
        : m_Id(targetId), m_Component(std::move(componentName)), m_ComponentYaml(std::move(componentYaml)) {
        const ComponentMetadata* metadata = ComponentRegistry::Get().FindByName(m_Component);
        if (Entity entity = scene.FindEntityByUUID(targetId)) {
            m_WasPresent = metadata != nullptr && metadata->has && metadata->has(entity);
            m_Before = EntitySnapshot::CaptureEntity(scene, targetId);
        }
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || m_ComponentYaml.empty()) {
            return;
        }
        Entity entity = scene->FindEntityByUUID(m_Id);
        if (!entity) {
            return;
        }
        try {
            const YAML::Node node = YAML::Load(m_ComponentYaml);
            ComponentSerializer::DeserializeCoreComponents(entity, node);
            ComponentSerializer::DeserializeHockeyMarkerComponents(entity, node);
            ComponentSerializer::DeserializeRenderComponents(entity, node);
            ComponentSerializer::DeserializeExternalComponents(entity, node);
        } catch (const std::exception& e) {
            HK_EDITOR_ERROR("PasteComponent parse failed: {}", e.what());
            return;
        }
        scene->RecalculateActiveInHierarchy(entity);
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        if (m_WasPresent) {
            EntitySnapshot::ApplyEntity(*scene, m_Before);
        } else {
            const ComponentMetadata* metadata = ComponentRegistry::Get().FindByName(m_Component);
            if (metadata != nullptr && metadata->remove) {
                if (Entity entity = scene->FindEntityByUUID(m_Id)) {
                    metadata->remove(entity);
                }
            }
        }
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Paste Component";
    }

private:
    UUID m_Id;
    std::string m_Component;
    std::string m_ComponentYaml;
    std::string m_Before;
    bool m_WasPresent = false;
};

class CreatePrefabCommand : public EditorCommand {
public:
    CreatePrefabCommand(UUID sourceId, std::filesystem::path path)
        : m_SourceId(sourceId), m_Path(std::move(path)) {}

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        Entity entity = scene->FindEntityByUUID(m_SourceId);
        if (!entity) {
            return;
        }
        if (!m_Saved) {
            // Capture the prior prefab-instance state so undo can restore it.
            m_HadPrefab = entity.HasComponent<PrefabComponent>();
            if (m_HadPrefab) {
                m_PrevPrefab = entity.GetComponent<PrefabComponent>();
            }
            UUID assetId;
            if (const Status status = PrefabSerializer::Save(*scene, entity, m_Path, assetId); !status) {
                HK_EDITOR_ERROR("Create prefab failed: {}", status.error);
                m_Failed = true;
                return;
            }
            m_AssetId = assetId;
            m_Saved = true;
        }
        // Link the source entity to the new prefab (Unity-style instance stamp).
        scene->Registry().emplace_or_replace<PrefabComponent>(entity.Handle(),
                                                              PrefabComponent{m_AssetId, m_SourceId, m_Path});
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        if (m_Failed) {
            return;
        }
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        Entity entity = scene->FindEntityByUUID(m_SourceId);
        if (!entity) {
            return;
        }
        if (m_HadPrefab) {
            scene->Registry().emplace_or_replace<PrefabComponent>(entity.Handle(), m_PrevPrefab);
        } else if (entity.HasComponent<PrefabComponent>()) {
            scene->Registry().remove<PrefabComponent>(entity.Handle());
        }
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Create Prefab";
    }

private:
    UUID m_SourceId;
    std::filesystem::path m_Path;
    UUID m_AssetId;
    PrefabComponent m_PrevPrefab;
    bool m_HadPrefab = false;
    bool m_Saved = false;
    bool m_Failed = false;
};

class RevertPrefabOverridesCommand : public EditorCommand {
public:
    RevertPrefabOverridesCommand(Scene& scene, UUID instanceId) : m_Id(instanceId) {
        // Snapshot the subtree's current values so undo can restore them.
        m_Before = EntitySnapshot::CaptureSubtree(scene, instanceId);
    }

    void Execute(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr) {
            return;
        }
        Entity entity = scene->FindEntityByUUID(m_Id);
        if (!entity) {
            return;
        }
        if (const Status status = PrefabSerializer::RevertInstanceToPrefab(*scene, entity); !status) {
            HK_EDITOR_ERROR("Revert prefab overrides failed: {}", status.error);
            return;
        }
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    void Undo(EditorContext& context) override {
        Scene* scene = context.activeScene;
        if (scene == nullptr || m_Before.empty()) {
            return;
        }
        // Restore the pre-revert field values for each entity in the subtree.
        try {
            const YAML::Node seq = YAML::Load(m_Before);
            if (seq.IsSequence()) {
                for (const auto node : seq) {
                    YAML::Emitter out;
                    out << node;
                    EntitySnapshot::ApplyEntity(*scene, out.c_str());
                }
            }
        } catch (const std::exception& e) {
            HK_EDITOR_ERROR("Revert undo failed: {}", e.what());
        }
        context.selection.Select(m_Id);
        context.MarkDirty();
    }

    std::string Name() const override {
        return "Revert Prefab Overrides";
    }

private:
    UUID m_Id;
    std::string m_Before;
};

} // namespace

namespace EditorCommands {

std::unique_ptr<EditorCommand> CreateEntity(std::string name, UUID parentId) {
    return std::make_unique<CreateEntityCommand>(std::move(name), parentId);
}

std::unique_ptr<EditorCommand> DeleteEntity(Scene& scene, UUID entityId) {
    return std::make_unique<DeleteEntityCommand>(scene, entityId);
}

std::unique_ptr<EditorCommand> DuplicateEntity(UUID sourceId) {
    return std::make_unique<DuplicateEntityCommand>(sourceId);
}

bool CanCreateEmptyParent(Scene& scene, const std::vector<UUID>& entityIds) {
    return !NormalizeCreateEmptyParentIds(scene, entityIds, nullptr, nullptr).empty();
}

std::unique_ptr<EditorCommand> CreateEmptyParent(Scene& scene, std::vector<UUID> entityIds) {
    return std::make_unique<CreateEmptyParentCommand>(scene, std::move(entityIds));
}

std::unique_ptr<EditorCommand> SpawnEntities(std::string actionName, SpawnBuilder builder) {
    return std::make_unique<SpawnEntitiesCommand>(std::move(actionName), std::move(builder));
}

std::unique_ptr<EditorCommand> InstantiatePrefab(std::filesystem::path prefabPath, UUID parentId) {
    auto builder = [path = std::move(prefabPath), parentId](Scene& scene) -> std::vector<UUID> {
        Result<Entity> root = PrefabSerializer::Instantiate(scene, path);
        if (!root) {
            HK_EDITOR_ERROR("Instantiate prefab '{}' failed: {}", path.string(), root.error);
            return {};
        }
        Entity rootEntity = root.value;
        if (parentId.IsValid()) {
            if (Entity parent = scene.FindEntityByUUID(parentId)) {
                scene.SetParent(rootEntity, parent);
            }
        }
        return {rootEntity.GetUUID()};
    };
    return std::make_unique<SpawnEntitiesCommand>("Instantiate Prefab", std::move(builder));
}

std::unique_ptr<EditorCommand> InstantiatePrefabAt(std::filesystem::path prefabPath, glm::vec3 worldPosition,
                                                   UUID parentId) {
    auto builder = [path = std::move(prefabPath), worldPosition, parentId](Scene& scene) -> std::vector<UUID> {
        Result<Entity> root = PrefabSerializer::Instantiate(scene, path);
        if (!root) {
            HK_EDITOR_ERROR("Instantiate prefab '{}' failed: {}", path.string(), root.error);
            return {};
        }
        Entity rootEntity = root.value;
        if (parentId.IsValid()) {
            if (Entity parent = scene.FindEntityByUUID(parentId)) {
                scene.SetParent(rootEntity, parent);
            }
        }
        // Override only the translation so the prefab's authored rotation/scale
        // survive the placement.
        glm::mat4 world = scene.GetWorldTransform(rootEntity);
        world[3] = glm::vec4(worldPosition, 1.0f);
        scene.SetWorldTransform(rootEntity, world);
        return {rootEntity.GetUUID()};
    };
    return std::make_unique<SpawnEntitiesCommand>("Instantiate Prefab", std::move(builder));
}

std::unique_ptr<EditorCommand> CreatePrefab(UUID sourceId, std::filesystem::path prefabPath) {
    return std::make_unique<CreatePrefabCommand>(sourceId, std::move(prefabPath));
}

std::unique_ptr<EditorCommand> RevertPrefabOverrides(Scene& scene, UUID instanceId) {
    return std::make_unique<RevertPrefabOverridesCommand>(scene, instanceId);
}

std::unique_ptr<EditorCommand> RenameEntity(UUID entityId, std::string oldName, std::string newName) {
    return std::make_unique<RenameEntityCommand>(entityId, std::move(oldName), std::move(newName));
}

std::unique_ptr<EditorCommand> SetActive(UUID entityId, bool oldValue, bool newValue) {
    return std::make_unique<SetActiveCommand>(entityId, oldValue, newValue);
}

std::unique_ptr<EditorCommand> SetParent(Scene& scene, UUID childId, UUID newParentId) {
    return std::make_unique<SetParentCommand>(scene, childId, newParentId);
}

std::unique_ptr<EditorCommand> MoveEntity(Scene& scene, UUID entityId, UUID newParentId, std::size_t siblingIndex) {
    return std::make_unique<MoveEntityCommand>(scene, entityId, newParentId, siblingIndex);
}

std::unique_ptr<EditorCommand> MoveEntities(Scene& scene, std::vector<UUID> entityIds, UUID newParentId,
                                            std::size_t siblingIndex) {
    return std::make_unique<MoveEntitiesCommand>(scene, std::move(entityIds), newParentId, siblingIndex);
}

std::unique_ptr<EditorCommand> TransformEntity(UUID entityId, const TransformData& oldValue,
                                               const TransformData& newValue) {
    return std::make_unique<TransformEntityCommand>(entityId, oldValue, newValue);
}

std::unique_ptr<EditorCommand> TransformEntities(std::vector<EntityTransformSnapshot> snapshots) {
    return std::make_unique<TransformEntitiesCommand>(std::move(snapshots));
}

std::unique_ptr<EditorCommand> AddComponent(UUID entityId, std::string componentName) {
    return std::make_unique<AddComponentCommand>(entityId, std::move(componentName));
}

std::unique_ptr<EditorCommand> RemoveComponent(Scene& scene, UUID entityId, std::string componentName) {
    return std::make_unique<RemoveComponentCommand>(scene, entityId, std::move(componentName));
}

std::unique_ptr<EditorCommand> EditComponentField(UUID entityId, std::string label, std::string beforeYaml,
                                                  std::string afterYaml) {
    return std::make_unique<EditComponentFieldCommand>(entityId, std::move(label), std::move(beforeYaml),
                                                       std::move(afterYaml));
}

std::unique_ptr<EditorCommand> PasteEntity(std::string subtreeYaml, UUID parentId) {
    return std::make_unique<PasteEntityCommand>(std::move(subtreeYaml), parentId);
}

std::unique_ptr<EditorCommand> PasteComponent(Scene& scene, UUID targetId, std::string componentName,
                                              std::string componentYaml) {
    return std::make_unique<PasteComponentCommand>(scene, targetId, std::move(componentName), std::move(componentYaml));
}

} // namespace EditorCommands

} // namespace Hockey
