#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

// Process-wide table of component metadata. Editors query it to list, add,
// remove and inspect components without compile-time knowledge of the types.
class ComponentRegistry {
public:
    static ComponentRegistry& Get();

    template <typename T> void RegisterComponent(ComponentMetadata metadata) {
        if (metadata.name.empty()) {
            metadata.name = ComponentName<T>();
        }
        if (!metadata.has) {
            metadata.has = [](Entity e) { return e.HasComponent<T>(); };
        }
        if (!metadata.add) {
            metadata.add = [](Entity e) {
                if (!e.HasComponent<T>()) {
                    e.AddComponent<T>();
                }
            };
        }
        if (!metadata.remove) {
            metadata.remove = [](Entity e) { e.RemoveComponent<T>(); };
        }
        if (!metadata.getData) {
            metadata.getData = [](Entity e) -> void* {
                return e.HasComponent<T>() ? static_cast<void*>(&e.GetComponent<T>()) : nullptr;
            };
        }
        m_Components.push_back(std::move(metadata));
    }

    const ComponentMetadata* FindByName(const std::string& name) const;
    const std::vector<ComponentMetadata>& All() const;

    // Registers every Phase 2 component. Idempotent (clears first).
    void RegisterPhase2Components();

private:
    std::vector<ComponentMetadata> m_Components;
};

} // namespace Hockey
