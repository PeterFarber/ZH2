#include "Hockey/Editor/Clipboard.hpp"

#include <exception>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"

namespace Hockey {

void Clipboard::CopyEntities(Scene& scene, const std::vector<UUID>& ids) {
    m_EntitySnapshots.clear();
    for (const UUID id : ids) {
        std::string snapshot = EntitySnapshot::CaptureSubtree(scene, id);
        if (!snapshot.empty()) {
            m_EntitySnapshots.push_back(std::move(snapshot));
        }
    }
}

void Clipboard::CopyComponent(Scene& scene, UUID entityId, const std::string& componentName) {
    m_ComponentName.clear();
    m_ComponentYaml.clear();

    const std::string entityYaml = EntitySnapshot::CaptureEntity(scene, entityId);
    if (entityYaml.empty()) {
        return;
    }
    try {
        const YAML::Node node = YAML::Load(entityYaml);
        const YAML::Node component = node[componentName];
        if (!component) {
            return;
        }
        // Re-emit just the single component as a one-key map so it can be applied
        // to any compatible entity later.
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << componentName << YAML::Value << component;
        out << YAML::EndMap;
        m_ComponentName = componentName;
        m_ComponentYaml = out.c_str();
    } catch (const std::exception& e) {
        HK_EDITOR_ERROR("Clipboard::CopyComponent parse failed: {}", e.what());
    }
}

void Clipboard::Clear() {
    m_EntitySnapshots.clear();
    m_ComponentName.clear();
    m_ComponentYaml.clear();
}

} // namespace Hockey
