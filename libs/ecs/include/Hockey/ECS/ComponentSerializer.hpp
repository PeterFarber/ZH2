#pragma once

#include <functional>

#include <yaml-cpp/yaml.h>

namespace Hockey {

class Entity;
class Scene;

// Manual (explicit) per-component YAML serialization. Phase 2 deliberately
// avoids metadata-driven auto serialization so the on-disk format is precise.
//
// Libraries layered above the ECS (e.g. hockey_physics) can serialize their own
// components without the ECS depending on them by installing external hooks via
// RegisterExternal(). Built-in components are always written first; external
// serializers append their keys to the same entity map.
class ComponentSerializer {
public:
    using SerializeFn = std::function<void(YAML::Emitter&, Entity)>;
    using DeserializeFn = std::function<void(Entity, const YAML::Node&)>;

    // Writes one entity as a YAML map node (Entity id + every present component).
    static void SerializeEntity(YAML::Emitter& out, Entity entity);
    // Appends only externally registered component keys to the current YAML map.
    static void SerializeExternalComponents(YAML::Emitter& out, Entity entity);

    // Creates an entity in 'scene' from a node, preserving its stored UUID.
    static Entity DeserializeEntity(Scene& scene, const YAML::Node& node);

    static bool DeserializeCoreComponents(Entity entity, const YAML::Node& node);
    static bool DeserializeHockeyMarkerComponents(Entity entity, const YAML::Node& node);
    static bool DeserializeRenderComponents(Entity entity, const YAML::Node& node);

    // Runs every registered external deserializer for the entity/node. Callers
    // that deserialize entities piecewise (e.g. prefab instancing) must invoke
    // this so externally-owned components are restored.
    static bool DeserializeExternalComponents(Entity entity, const YAML::Node& node);

    // Extension seam. 'serialize' may be null if a library only deserializes.
    static void RegisterExternal(SerializeFn serialize, DeserializeFn deserialize);
    static void ClearExternal();
};

} // namespace Hockey
