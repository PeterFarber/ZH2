#pragma once

#include <filesystem>
#include <string>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

class Scene;
class Entity;

// Scene entity tree: displays roots and children recursively, supports
// selection (single + Ctrl multi-select), inline rename, a right-click context
// menu (create/duplicate/delete/unparent) and drag-and-drop reparenting. The
// selection is kept in EditorContext so the inspector/viewport stay in sync.
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
        CreateChild,
        Duplicate,
        Delete,
        Unparent,
        Reparent,
        InstantiatePrefab,
        CreatePrefab,
    };

    void DrawEntityNode(EditorContext& context, Scene& scene, const Entity& entity);
    void DrawRootDropTarget();
    void ApplyPending(EditorContext& context, Scene& scene);
    void BeginRename(UUID id, const std::string& currentName);

    PendingKind m_PendingKind = PendingKind::None;
    UUID m_PendingTarget{0};
    UUID m_PendingParent{0};
    std::filesystem::path m_PendingPrefabPath;

    UUID m_RenamingId{0};
    bool m_RenameFocus = false;
    char m_RenameBuffer[256] = {};
    std::string m_RenameOriginal;
};

} // namespace Hockey
