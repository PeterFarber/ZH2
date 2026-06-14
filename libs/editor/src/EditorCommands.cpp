#include "Hockey/Editor/EditorCommands.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <functional>
#include <unordered_map>
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
            Entity copy = scene->DuplicateEntity(source);
            if (!copy) {
                return;
            }
            m_NewId = copy.GetUUID();
            if (Entity parent = scene->GetParent(copy)) {
                m_ParentId = parent.GetUUID();
            }
            m_Snapshot = EntitySnapshot::CaptureSubtree(*scene, m_NewId);
            m_Captured = true;
        } else {
            m_NewId = EntitySnapshot::RestoreSubtree(*scene, m_Snapshot, m_ParentId);
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
    UUID m_SourceId;
    UUID m_NewId{0};
    UUID m_ParentId{0};
    std::string m_Snapshot;
    bool m_Captured = false;
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

std::unique_ptr<EditorCommand> RenameEntity(UUID entityId, std::string oldName, std::string newName) {
    return std::make_unique<RenameEntityCommand>(entityId, std::move(oldName), std::move(newName));
}

std::unique_ptr<EditorCommand> SetActive(UUID entityId, bool oldValue, bool newValue) {
    return std::make_unique<SetActiveCommand>(entityId, oldValue, newValue);
}

std::unique_ptr<EditorCommand> SetParent(Scene& scene, UUID childId, UUID newParentId) {
    return std::make_unique<SetParentCommand>(scene, childId, newParentId);
}

std::unique_ptr<EditorCommand> TransformEntity(UUID entityId, const TransformData& oldValue,
                                               const TransformData& newValue) {
    return std::make_unique<TransformEntityCommand>(entityId, oldValue, newValue);
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
