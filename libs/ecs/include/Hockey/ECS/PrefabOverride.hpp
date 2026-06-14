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

    Status Apply(Scene& scene);

private:
    std::vector<PrefabOverride> m_Overrides;
};

} // namespace Hockey
