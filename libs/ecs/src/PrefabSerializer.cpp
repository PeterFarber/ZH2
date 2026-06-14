#include "Hockey/ECS/PrefabSerializer.hpp"

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

Status PrefabSerializer::Save(Scene& scene, Entity root, const std::filesystem::path& path) {
    if (!scene.IsValid(root)) {
        return Status::Fail("Prefab save: invalid root entity");
    }

    entt::registry& registry = scene.Registry();
    const entt::entity rootHandle = root.Handle();

    std::vector<Entity> entities;
    std::function<void(Entity)> collect = [&](Entity entity) {
        entities.push_back(entity);
        for (Entity child : scene.GetChildren(entity)) {
            collect(child);
        }
    };
    collect(root);

    // A prefab root is parentless by definition; temporarily drop any external
    // parent link so the saved file does not reference entities outside the tree.
    const bool rootHadParent = registry.all_of<ParentComponent>(rootHandle);
    ParentComponent savedParent;
    if (rootHadParent) {
        savedParent = registry.get<ParentComponent>(rootHandle);
        registry.remove<ParentComponent>(rootHandle);
    }

    const UUID assetId;

    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "Prefab" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << root.GetName();
    out << YAML::Key << "Version" << YAML::Value << kPrefabVersion;
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

} // namespace Hockey
