#pragma once

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class Scene;

// A single per-instance edit applied on top of a prefab's authored values.
struct PrefabOverride {
    UUID entityId;
    std::string componentName;
    std::string fieldName;
    YAML::Node value;
};

// Collection of overrides applied to a scene after a prefab is instantiated.
class PrefabOverrideSet {
public:
    void AddOverride(const PrefabOverride& override);
    const std::vector<PrefabOverride>& Overrides() const;

    void Clear();
    bool Empty() const;

    Status Apply(Scene& scene);

    // Persistence. The override records are stored alongside the scene (the
    // edited component values are already baked into the entity data, so these
    // records track *which* fields diverge from the prefab for display/revert).
    // Serialize emits a YAML sequence; Deserialize replaces the current set.
    void Serialize(YAML::Emitter& out) const;
    void Deserialize(const YAML::Node& node);

private:
    std::vector<PrefabOverride> m_Overrides;
};

} // namespace Hockey
