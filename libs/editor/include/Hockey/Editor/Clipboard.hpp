#pragma once

#include <string>
#include <vector>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class Scene;

// Editor-internal clipboard. Entities and component data are stored as YAML so
// they survive deleting the source (cut) and can be re-instantiated with fresh
// UUIDs. OS clipboard integration is intentionally out of scope for Phase 4.
class Clipboard {
public:
    // ----- entities ---------------------------------------------------------
    // Captures each id's subtree (depth-first) as an independent snapshot.
    void CopyEntities(Scene& scene, const std::vector<UUID>& ids);
    bool HasEntities() const {
        return !m_EntitySnapshots.empty();
    }
    const std::vector<std::string>& EntitySnapshots() const {
        return m_EntitySnapshots;
    }

    // ----- single component -------------------------------------------------
    // Captures one component's data (as a YAML map keyed by the component name).
    void CopyComponent(Scene& scene, UUID entityId, const std::string& componentName);
    bool HasComponent() const {
        return !m_ComponentName.empty();
    }
    const std::string& ComponentName() const {
        return m_ComponentName;
    }
    const std::string& ComponentYaml() const {
        return m_ComponentYaml;
    }

    void Clear();

private:
    std::vector<std::string> m_EntitySnapshots;
    std::string m_ComponentName;
    std::string m_ComponentYaml;
};

} // namespace Hockey
