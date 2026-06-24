#pragma once

#include <cstddef>
#include <vector>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class Scene;

// Tracks the editor's selected entities by UUID (stable across reloads), with a
// distinguished "primary" selection used by single-target operations and the
// inspector. Pure logic with no ImGui/ECS coupling so it is unit-testable.
class Selection {
public:
    // Replaces the selection with a single entity (clears when id is invalid).
    void Select(UUID entityId);
    // Selects every entity in the scene; the last one becomes primary.
    void SelectAll(Scene& scene);
    // Adds an entity and makes it primary (no-op for invalid/duplicate ids).
    void Add(UUID entityId);
    void Remove(UUID entityId);
    // Adds the entity if absent, otherwise removes it.
    void Toggle(UUID entityId);
    // Selects the inclusive range between the current range anchor and target
    // in the caller-provided visible row order. Additive mode preserves the
    // existing selection outside the range.
    void SelectRange(const std::vector<UUID>& orderedVisibleIds, UUID targetId, bool additive);
    void Clear();

    bool IsSelected(UUID entityId) const;
    // Primary selection, or an invalid UUID when nothing is selected.
    UUID Primary() const;
    UUID RangeAnchor() const;
    const std::vector<UUID>& All() const;

    std::size_t Count() const;
    bool Empty() const;

    // Drops selected ids no longer present in the scene (e.g. after deletion or
    // reload) and repairs the primary selection.
    void Validate(const Scene& scene);

private:
    std::vector<UUID> m_Selected;
    UUID m_Primary{0};
    UUID m_RangeAnchor{0};
};

} // namespace Hockey
