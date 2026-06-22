#include "Hockey/ECS/SceneSerializer.hpp"

#include <cstdint>
#include <exception>
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

SceneSerializer::SceneSerializer(Scene& scene) : m_Scene(scene) {}

Status SceneSerializer::Serialize(const std::filesystem::path& path) {
    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "Scene" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << m_Scene.GetName();
    out << YAML::Key << "Mode" << YAML::Value << std::string(SceneModeToString(m_Scene.GetMode()));
    out << YAML::Key << "Version" << YAML::Value << kSceneVersion;
    out << YAML::Key << "RootOrder" << YAML::Value << YAML::BeginSeq;
    for (Entity root : m_Scene.GetRootEntities()) {
        out << root.GetUUID().Value();
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    for (Entity entity : m_Scene.GetAllEntities()) {
        ComponentSerializer::SerializeEntity(out, entity);
    }
    out << YAML::EndSeq;

    // Prefab instance overrides (only written when present, keeping legacy
    // scene files byte-compatible).
    if (!m_Scene.PrefabOverrides().Empty()) {
        out << YAML::Key << "PrefabOverrides" << YAML::Value;
        m_Scene.PrefabOverrides().Serialize(out);
    }

    out << YAML::EndMap;

    if (!out.good()) {
        const std::string error = "Scene YAML emit failed: " + out.GetLastError();
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    if (const auto parent = path.parent_path(); !parent.empty()) {
        if (const Status created = FileSystem::CreateDirectories(parent); !created) {
            HK_CORE_ERROR("Failed to create scene directory: {}", created.error);
            return created;
        }
    }

    if (const Status written = FileSystem::WriteTextFile(path, out.c_str()); !written) {
        HK_CORE_ERROR("Failed to write scene file '{}': {}", path.string(), written.error);
        return written;
    }

    HK_CORE_INFO("Saved scene '{}' ({} entities) to {}", m_Scene.GetName(), m_Scene.EntityCount(), path.string());
    return Status::Ok();
}

Status SceneSerializer::Deserialize(const std::filesystem::path& path) {
    const Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        const std::string error = "Failed to read scene file: " + path.string();
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    YAML::Node root;
    try {
        root = YAML::Load(text.value);
    } catch (const std::exception& e) {
        const std::string error = std::string("Scene YAML parse error: ") + e.what();
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    if (!root["Entities"]) {
        const std::string error = "Scene file missing 'Entities' section";
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    m_Scene.Clear();

    std::vector<UUID> serializedRootOrder;
    bool hasSerializedRootOrder = false;

    if (const auto sceneNode = root["Scene"]; sceneNode) {
        if (sceneNode["Name"]) {
            m_Scene.SetName(sceneNode["Name"].as<std::string>());
        }
        // Mode is optional for backward compatibility: scenes written before
        // mode serialization (or hand-authored without it) load as Edit.
        if (sceneNode["Mode"]) {
            m_Scene.SetMode(SceneModeFromString(sceneNode["Mode"].as<std::string>()));
        } else {
            m_Scene.SetMode(SceneMode::Edit);
        }
        if (const auto rootOrderNode = sceneNode["RootOrder"]) {
            hasSerializedRootOrder = true;
            for (const auto idNode : rootOrderNode) {
                serializedRootOrder.push_back(UUID(idNode.as<std::uint64_t>()));
            }
        }
    }

    try {
        for (const auto entityNode : root["Entities"]) {
            ComponentSerializer::DeserializeEntity(m_Scene, entityNode);
        }
    } catch (const std::exception& e) {
        const std::string error = std::string("Scene entity parse error: ") + e.what();
        HK_CORE_ERROR("{}", error);
        return Status::Fail(error);
    }

    // Consistency pass: drop hierarchy references that point at missing entities.
    entt::registry& registry = m_Scene.Registry();
    for (Entity entity : m_Scene.GetAllEntities()) {
        const entt::entity handle = entity.Handle();
        if (auto* parent = registry.try_get<ParentComponent>(handle)) {
            if (!m_Scene.ContainsUUID(parent->parentId)) {
                HK_CORE_WARN("Scene load: entity {} references missing parent; detaching", entity.GetUUID().ToString());
                registry.remove<ParentComponent>(handle);
            }
        }
        if (auto* children = registry.try_get<ChildrenComponent>(handle)) {
            auto& list = children->children;
            std::vector<UUID> kept;
            kept.reserve(list.size());
            for (const UUID childId : list) {
                if (m_Scene.ContainsUUID(childId)) {
                    kept.push_back(childId);
                } else {
                    HK_CORE_WARN("Scene load: entity {} references missing child", entity.GetUUID().ToString());
                }
            }
            list = std::move(kept);
        }
    }

    if (hasSerializedRootOrder) {
        m_Scene.SetRootEntityOrder(serializedRootOrder);
    } else {
        std::vector<UUID> legacyRootOrder;
        for (Entity rootEntity : m_Scene.GetRootEntities()) {
            legacyRootOrder.push_back(rootEntity.GetUUID());
        }
        m_Scene.SetRootEntityOrder(legacyRootOrder);
    }

    m_Scene.RecalculateAllActiveInHierarchy();

    // Restore prefab override records (optional section). Values are already
    // baked into the entity data above; these track which fields diverge.
    if (const auto overridesNode = root["PrefabOverrides"]) {
        m_Scene.PrefabOverrides().Deserialize(overridesNode);
    } else {
        m_Scene.PrefabOverrides().Clear();
    }

    HK_CORE_INFO("Loaded scene '{}' ({} entities) from {}", m_Scene.GetName(), m_Scene.EntityCount(), path.string());
    return Status::Ok();
}

} // namespace Hockey
