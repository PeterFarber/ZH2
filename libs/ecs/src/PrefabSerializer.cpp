#include "Hockey/ECS/PrefabSerializer.hpp"

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

namespace {

// Depth-first collection of an entity and all of its descendants.
void CollectSubtree(Scene& scene, Entity root, std::vector<Entity>& out) {
    out.push_back(root);
    for (Entity child : scene.GetChildren(root)) {
        CollectSubtree(scene, child, out);
    }
}

// Emits a prefab file for the subtree rooted at `root`, stamping it with the
// given asset id. Used by both Save (fresh id) and ApplyInstanceToPrefab
// (preserves the existing prefab id so live instances stay linked).
Status WritePrefabFile(Scene& scene, Entity root, const std::filesystem::path& path, const UUID& assetId) {
    if (!scene.IsValid(root)) {
        return Status::Fail("Prefab save: invalid root entity");
    }

    entt::registry& registry = scene.Registry();
    const entt::entity rootHandle = root.Handle();

    std::vector<Entity> entities;
    CollectSubtree(scene, root, entities);

    // A prefab root is parentless by definition; temporarily drop any external
    // parent link so the saved file does not reference entities outside the tree.
    const bool rootHadParent = registry.all_of<ParentComponent>(rootHandle);
    ParentComponent savedParent;
    if (rootHadParent) {
        savedParent = registry.get<ParentComponent>(rootHandle);
        registry.remove<ParentComponent>(rootHandle);
    }

    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "Prefab" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << root.GetName();
    out << YAML::Key << "Version" << YAML::Value << PrefabSerializer::kPrefabVersion;
    out << YAML::Key << "AssetId" << YAML::Value << assetId.Value();
    out << YAML::EndMap;

    out << YAML::Key << "Root" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Entity" << YAML::Value << root.GetUUID().Value();
    out << YAML::EndMap;

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    for (Entity entity : entities) {
        ComponentSerializer::SerializeEntity(out, entity);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;

    if (rootHadParent) {
        registry.emplace_or_replace<ParentComponent>(rootHandle, savedParent);
    }

    if (!out.good()) {
        const std::string error = "Prefab YAML emit failed: " + out.GetLastError();
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    if (const auto parent = path.parent_path(); !parent.empty()) {
        if (const Status created = FileSystem::CreateDirectories(parent); !created) {
            return created;
        }
    }

    if (const Status written = FileSystem::WriteTextFile(path, out.c_str()); !written) {
        HK_CORE_ERROR("Failed to write prefab '{}': {}", path.string(), written.error);
        return written;
    }

    HK_CORE_INFO("Saved prefab '{}' ({} entities) to {}", root.GetName(), entities.size(), path.string());
    return Status::Ok();
}

// Parsed prefab file indexed by each entity's stored ("source") id. The owning
// `root` node keeps the per-entity nodes alive.
struct LoadedPrefab {
    YAML::Node root;
    std::unordered_map<std::uint64_t, YAML::Node> byId;
};

Result<LoadedPrefab> LoadPrefabNodes(const std::filesystem::path& path) {
    const Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        return Result<LoadedPrefab>::Fail("Failed to read prefab file: " + path.string());
    }
    LoadedPrefab loaded;
    try {
        loaded.root = YAML::Load(text.value);
    } catch (const std::exception& e) {
        return Result<LoadedPrefab>::Fail(std::string("Prefab YAML parse error: ") + e.what());
    }
    if (!loaded.root["Entities"]) {
        return Result<LoadedPrefab>::Fail("Prefab file missing 'Entities' section");
    }
    for (const auto node : loaded.root["Entities"]) {
        if (node["Entity"]) {
            loaded.byId.emplace(node["Entity"].as<std::uint64_t>(), node);
        }
    }
    return Result<LoadedPrefab>::Ok(std::move(loaded));
}

// Components whose fields PrefabOverrideSet::Apply can re-apply. Restricting the
// diff to these keeps every recorded override round-trippable.
const std::unordered_set<std::string>& OverridableComponents() {
    static const std::unordered_set<std::string> kComponents = {
        "TransformComponent",   "NameComponent",        "ObjectSettingsComponent", "ActiveComponent",
        "TeamComponent",        "PlayerRoleComponent",  "GoalComponent",           "PuckComponent",
        "SpawnPointComponent",  "FaceoffSpotComponent", "RinkComponent",           "PlayAreaComponent",
        "CameraRigMarkerComponent"};
    return kComponents;
}

// Serializes a live entity to a standalone YAML map node (matching the prefab
// file's per-entity layout) so it can be diffed against the authored node.
YAML::Node EntityToNode(Entity entity) {
    YAML::Emitter out;
    ComponentSerializer::SerializeEntity(out, entity);
    return YAML::Load(out.c_str());
}

} // namespace

Status PrefabSerializer::Save(Scene& scene, Entity root, const std::filesystem::path& path) {
    UUID ignored;
    return Save(scene, root, path, ignored);
}

Status PrefabSerializer::Save(Scene& scene, Entity root, const std::filesystem::path& path, UUID& outAssetId) {
    const UUID assetId;
    outAssetId = assetId;
    return WritePrefabFile(scene, root, path, assetId);
}

Result<Entity> PrefabSerializer::Instantiate(Scene& targetScene, const std::filesystem::path& path) {
    const Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        return Result<Entity>::Fail("Failed to read prefab file: " + path.string());
    }

    YAML::Node root;
    try {
        root = YAML::Load(text.value);
    } catch (const std::exception& e) {
        return Result<Entity>::Fail(std::string("Prefab YAML parse error: ") + e.what());
    }

    if (!root["Entities"] || !root["Root"] || !root["Root"]["Entity"]) {
        return Result<Entity>::Fail("Prefab file missing 'Root'/'Entities' sections");
    }

    UUID assetId;
    if (const auto prefabNode = root["Prefab"]; prefabNode && prefabNode["AssetId"]) {
        assetId = UUID(prefabNode["AssetId"].as<std::uint64_t>());
    }

    std::unordered_map<std::uint64_t, std::uint64_t> remap;
    entt::registry& registry = targetScene.Registry();

    try {
        // Pass 1: create entities with fresh UUIDs and load their components.
        for (const auto node : root["Entities"]) {
            const std::uint64_t oldId = node["Entity"].as<std::uint64_t>();

            UUID newId;
            while (targetScene.ContainsUUID(newId) || remap.find(newId.Value()) != remap.end()) {
                newId = UUID();
            }
            remap[oldId] = newId.Value();

            Entity entity = targetScene.CreateEntityWithUUID(newId, "GameObject");
            ComponentSerializer::DeserializeCoreComponents(entity, node);
            ComponentSerializer::DeserializeHockeyMarkerComponents(entity, node);
            ComponentSerializer::DeserializeRenderComponents(entity, node);
            ComponentSerializer::DeserializeExternalComponents(entity, node);
        }

        // Pass 2: remap hierarchy references and stamp the PrefabComponent.
        for (const auto node : root["Entities"]) {
            const std::uint64_t oldId = node["Entity"].as<std::uint64_t>();
            const UUID newId(remap.at(oldId));
            Entity entity = targetScene.FindEntityByUUID(newId);
            if (!entity.IsValid()) {
                continue;
            }
            const entt::entity handle = entity.Handle();

            if (auto* parent = registry.try_get<ParentComponent>(handle)) {
                const auto it = remap.find(parent->parentId.Value());
                if (it != remap.end()) {
                    parent->parentId = UUID(it->second);
                } else {
                    registry.remove<ParentComponent>(handle);
                }
            }

            if (auto* children = registry.try_get<ChildrenComponent>(handle)) {
                std::vector<UUID> remapped;
                remapped.reserve(children->children.size());
                for (const UUID childId : children->children) {
                    const auto it = remap.find(childId.Value());
                    if (it != remap.end()) {
                        remapped.push_back(UUID(it->second));
                    }
                }
                children->children = std::move(remapped);
            }

            PrefabComponent prefab;
            prefab.prefabAssetId = assetId;
            prefab.sourceEntityId = UUID(oldId);
            prefab.sourcePath = path;
            registry.emplace_or_replace<PrefabComponent>(handle, prefab);
        }
    } catch (const std::exception& e) {
        return Result<Entity>::Fail(std::string("Prefab instancing error: ") + e.what());
    }

    const std::uint64_t rootOldId = root["Root"]["Entity"].as<std::uint64_t>();
    const auto rootIt = remap.find(rootOldId);
    if (rootIt == remap.end()) {
        return Result<Entity>::Fail("Prefab root entity not found in entity list");
    }

    targetScene.RecalculateAllActiveInHierarchy();

    Entity rootEntity = targetScene.FindEntityByUUID(UUID(rootIt->second));
    HK_CORE_INFO("Instantiated prefab '{}' from {}", rootEntity.GetName(), path.string());
    return Result<Entity>::Ok(rootEntity);
}

Result<int> PrefabSerializer::ComputeOverrides(Scene& scene, Entity instanceRoot, PrefabOverrideSet& outSet) {
    outSet.Clear();
    if (!scene.IsValid(instanceRoot) || !instanceRoot.HasComponent<PrefabComponent>()) {
        return Result<int>::Fail("ComputeOverrides: entity is not a prefab instance");
    }
    const std::filesystem::path source = instanceRoot.GetComponent<PrefabComponent>().sourcePath;
    Result<LoadedPrefab> prefab = LoadPrefabNodes(source);
    if (!prefab) {
        return Result<int>::Fail(prefab.error);
    }

    std::vector<Entity> entities;
    CollectSubtree(scene, instanceRoot, entities);

    int count = 0;
    for (Entity entity : entities) {
        if (!entity.HasComponent<PrefabComponent>()) {
            continue; // entity added to the instance: nothing to diff against
        }
        const auto fileIt = prefab.value.byId.find(entity.GetComponent<PrefabComponent>().sourceEntityId.Value());
        if (fileIt == prefab.value.byId.end()) {
            continue;
        }
        const YAML::Node& fileNode = fileIt->second;
        const YAML::Node instNode = EntityToNode(entity);
        const UUID entityId = entity.GetUUID();

        for (const std::string& component : OverridableComponents()) {
            const YAML::Node instComp = instNode[component];
            if (!instComp || !instComp.IsMap()) {
                continue;
            }
            const YAML::Node fileComp = fileNode[component];
            for (const auto field : instComp) {
                const std::string fieldName = field.first.as<std::string>();
                const YAML::Node fileValue = fileComp ? fileComp[fieldName] : YAML::Node();
                if (!fileValue || YAML::Dump(field.second) != YAML::Dump(fileValue)) {
                    outSet.AddOverride({entityId, component, fieldName, field.second});
                    ++count;
                }
            }
        }
    }
    return Result<int>::Ok(count);
}

Status PrefabSerializer::ApplyInstanceToPrefab(Scene& scene, Entity instanceRoot) {
    if (!scene.IsValid(instanceRoot) || !instanceRoot.HasComponent<PrefabComponent>()) {
        return Status::Fail("Apply prefab: entity is not a prefab instance");
    }
    const PrefabComponent prefab = instanceRoot.GetComponent<PrefabComponent>();
    if (prefab.sourcePath.empty()) {
        return Status::Fail("Apply prefab: instance has no source path");
    }

    // Write the instance subtree back to the prefab, preserving its asset id so
    // other instances keep their link.
    if (const Status status = WritePrefabFile(scene, instanceRoot, prefab.sourcePath, prefab.prefabAssetId); !status) {
        return status;
    }

    // The file now keys entities by their current instance ids, so refresh each
    // instance entity's source id to match (keeps later reverts aligned).
    entt::registry& registry = scene.Registry();
    std::vector<Entity> entities;
    CollectSubtree(scene, instanceRoot, entities);
    for (Entity entity : entities) {
        if (auto* component = registry.try_get<PrefabComponent>(entity.Handle())) {
            component->sourceEntityId = entity.GetUUID();
        }
    }
    HK_CORE_INFO("Applied prefab instance '{}' to {}", instanceRoot.GetName(), prefab.sourcePath.string());
    return Status::Ok();
}

Status PrefabSerializer::RevertInstanceToPrefab(Scene& scene, Entity instanceRoot) {
    if (!scene.IsValid(instanceRoot) || !instanceRoot.HasComponent<PrefabComponent>()) {
        return Status::Fail("Revert prefab: entity is not a prefab instance");
    }
    const std::filesystem::path source = instanceRoot.GetComponent<PrefabComponent>().sourcePath;
    Result<LoadedPrefab> prefab = LoadPrefabNodes(source);
    if (!prefab) {
        return Status::Fail(prefab.error);
    }

    entt::registry& registry = scene.Registry();
    std::vector<Entity> entities;
    CollectSubtree(scene, instanceRoot, entities);

    int reverted = 0;
    for (Entity entity : entities) {
        if (!entity.HasComponent<PrefabComponent>()) {
            continue;
        }
        const auto fileIt = prefab.value.byId.find(entity.GetComponent<PrefabComponent>().sourceEntityId.Value());
        if (fileIt == prefab.value.byId.end()) {
            continue;
        }
        const YAML::Node& node = fileIt->second;
        const entt::entity handle = entity.Handle();

        // Preserve identity, hierarchy and the prefab link; only authored
        // component *values* are restored.
        const bool hadParent = registry.all_of<ParentComponent>(handle);
        ParentComponent parentBackup;
        if (hadParent) {
            parentBackup = registry.get<ParentComponent>(handle);
        }
        const ChildrenComponent childrenBackup = registry.get<ChildrenComponent>(handle);
        const PrefabComponent prefabBackup = registry.get<PrefabComponent>(handle);

        ComponentSerializer::DeserializeCoreComponents(entity, node);
        ComponentSerializer::DeserializeHockeyMarkerComponents(entity, node);
        ComponentSerializer::DeserializeRenderComponents(entity, node);
        ComponentSerializer::DeserializeExternalComponents(entity, node);

        if (hadParent) {
            registry.emplace_or_replace<ParentComponent>(handle, parentBackup);
        } else if (registry.all_of<ParentComponent>(handle)) {
            registry.remove<ParentComponent>(handle);
        }
        registry.get<ChildrenComponent>(handle).children = childrenBackup.children;
        registry.emplace_or_replace<PrefabComponent>(handle, prefabBackup);
        ++reverted;
    }

    scene.RecalculateAllActiveInHierarchy();
    // The instance now matches the prefab, so any recorded divergence is stale.
    scene.PrefabOverrides().Clear();
    HK_CORE_INFO("Reverted prefab instance '{}' ({} entities) to {}", instanceRoot.GetName(), reverted,
                 source.string());
    return Status::Ok();
}

} // namespace Hockey
