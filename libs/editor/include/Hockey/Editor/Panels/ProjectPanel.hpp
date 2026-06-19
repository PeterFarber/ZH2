#pragma once

#include <filesystem>
#include <string>

#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

namespace Hockey {

// Project asset/file browser over data/raw, data/editor and data/shaders. Shows
// a directory tree (or a flat filtered list while searching), a details pane for
// the selected file, and file operations (new folder, rename, delete, reveal).
// Scene loading and prefab instancing are wired in the scene-management step.
class ProjectPanel : public Panel {
public:
    ProjectPanel();
    void OnImGui(EditorContext& context) override;

private:
    enum class Modal {
        None,
        CreateFolder,
        Rename,
        ConfirmDelete,
    };

    void DrawToolbar(EditorContext& context);
    void DrawTree(EditorContext& context);
    void DrawSearchResults(EditorContext& context);
    void DrawDirectoryChildren(EditorContext& context, const std::filesystem::path& dir);
    void DrawFileLeaf(EditorContext& context, const ProjectEntry& entry);
    void DrawContextMenu(EditorContext& context, const ProjectEntry& entry);
    void DrawDetails(EditorContext& context);
    void DrawModals(EditorContext& context);

    // Drag a prefab file (path payload) or an imported asset (asset-id payload)
    // out of the panel; double-click opens scenes / instances prefabs.
    void BeginFileDragSource(EditorContext& context, const ProjectEntry& entry);
    // Asset-pipeline context-menu actions (import/cook/recook/validate/copy id),
    // shown when an AssetManager is wired and the file is an importable asset.
    void DrawAssetActions(EditorContext& context, const std::filesystem::path& path);
    // PBR material editor shown in the details pane for a selected .material.yaml,
    // with save (reimport + recook) support.
    void DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path);
    // Pushes the in-progress material edits to the renderer as a live preview
    // (no disk write/recook) so the viewport updates while editing.
    void ApplyMaterialPreview(EditorContext& context);
    // Creates a new default material asset under `directory` and imports it.
    void CreateMaterialAsset(EditorContext& context, const std::filesystem::path& directory);
    void HandleFileActivation(EditorContext& context, const ProjectEntry& entry);
    void OpenSceneFile(EditorContext& context, const std::filesystem::path& path);
    void InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path);

    void RequestModal(Modal modal, const std::filesystem::path& target);

    ProjectBrowser m_Browser;

    Modal m_Modal = Modal::None;
    bool m_OpenModalRequested = false;
    std::filesystem::path m_ModalTarget;
    char m_NameBuffer[256] = {};
    std::string m_Status;

    // Cached material being edited in the details pane (loaded lazily when the
    // selected .material.yaml changes), plus its on-disk path and AssetID. While
    // editing, changes are pushed to the renderer as a live preview; switching
    // away (or Revert) discards an unsaved preview via InvalidateAsset.
    MaterialSource m_EditMaterial;
    std::filesystem::path m_EditMaterialPath;
    uint64_t m_EditMaterialId = 0;
    bool m_MaterialPreviewActive = false;
};

} // namespace Hockey
