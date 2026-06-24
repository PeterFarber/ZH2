#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

class Scene;
class Entity;

// Scene entity tree: displays roots and children recursively, supports
// selection (single + Ctrl multi-select), inline rename, a right-click context
// menu (create/duplicate/delete/unparent) and drag-and-drop reparenting/sorting.
// The selection is kept in EditorContext so the inspector/viewport stay in sync.
class HierarchyPanel : public Panel {
public:
    HierarchyPanel();
    void OnImGui(EditorContext& context) override;

private:
    // Structural scene edits are deferred until after the tree is drawn so the
    // registry is not mutated mid-iteration.
    enum class PendingKind {
        None,
        CreateEmpty,
        CreateEmptyParent,
        CreateChild,
        Duplicate,
        Delete,
        Unparent,
        MoveEntity,
        InstantiatePrefab,
        CreatePrefab,
    };

    void DrawEntityNode(EditorContext& context, Scene& scene, const Entity& entity,
                        const std::unordered_set<std::uint64_t>* visibleFilter, bool searchActive);
    void DrawSearchToolbar();
    void DrawRootDropTarget(EditorContext& context, Scene& scene);
    void ApplyPending(EditorContext& context, Scene& scene);
    void BeginRename(UUID id, const std::string& currentName);
    void DropStaleExpandedIds(const Scene& scene);
    bool IsEntityExpanded(UUID id) const;
    void SetEntityExpanded(UUID id, bool expanded);
    void SetEntityExpandedRecursive(Scene& scene, const Entity& entity, bool expanded);

    PendingKind m_PendingKind = PendingKind::None;
    UUID m_PendingTarget{0};
    UUID m_PendingParent{0};
    std::size_t m_PendingSiblingIndex = 0;
    std::filesystem::path m_PendingPrefabPath;
    std::vector<UUID> m_PendingMoveTargets;

    UUID m_RenamingId{0};
    bool m_RenameFocus = false;
    char m_RenameBuffer[256] = {};
    std::string m_RenameOriginal;

    std::array<char, 128> m_SearchBuffer{};
    bool m_FocusSearchNextFrame = false;

    std::unordered_set<std::uint64_t> m_ExpandedEntityIds;
    std::vector<UUID> m_VisibleEntityRows;
};

} // namespace Hockey
