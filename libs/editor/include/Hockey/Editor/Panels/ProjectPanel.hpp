#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

namespace Hockey {

struct AssetMetadata;

// Project asset/file browser over data/raw, data/editor and data/shaders. Shows
// a directory tree (or a flat filtered list while searching), a details pane for
// the selected file, and file operations (new folder, rename, delete, reveal).
// Scene loading and prefab instancing are wired in the scene-management step.
class ProjectPanel : public Panel {
public:
    ProjectPanel();
    void OnImGui(EditorContext& context) override;

private:
    enum class ProjectSearchScope {
        All,
        SelectedRoot,
        SelectedFolder,
    };

    enum class Modal {
        None,
        CreateFolder,
        Rename,
        ConfirmDelete,
    };

    void EnsureSelectionState(EditorContext& context);
    void DrawToolbar(EditorContext& context);
    void DrawFolderTree(EditorContext& context);
    void DrawFolderNode(EditorContext& context, const CookedProjectEntry& folder);
    void DrawFolderContents(EditorContext& context);
    void DrawSearchResults(EditorContext& context);
    void DrawCookedEntry(EditorContext& context, const CookedProjectEntry& entry);
    void DrawCookedContextMenu(EditorContext& context, const CookedProjectEntry& entry);
    void DrawDetails(EditorContext& context);
    void DrawStatusStrip(EditorContext& context);
    void DrawViewSizeSlider();
    void DrawModals(EditorContext& context);

    // Drag a prefab file (path payload) or an imported asset (asset-id payload)
    // out of the panel; double-click opens scenes / instances prefabs.
    void BeginCookedDragSource(EditorContext& context, const CookedProjectEntry& entry);
    void HandleCookedActivation(EditorContext& context, const CookedProjectEntry& entry);
    void ReimportAndRecook(EditorContext& context, const CookedProjectEntry& entry);
    void Recook(EditorContext& context, const CookedProjectEntry& entry);
    void ImportSelectedFile(EditorContext& context);
    void ImportExternalAsset(EditorContext& context, const std::filesystem::path& source);
    bool ImportAndCookRawAsset(EditorContext& context, const std::filesystem::path& rawPath);
    void ImportAllAndCook(EditorContext& context);
    void RecookDirty(EditorContext& context);
    void InvalidateAssetEvents(EditorContext& context);
    std::vector<CookedProjectEntry> SearchEntries(EditorContext& context, bool* truncated) const;
    bool MatchesSearchAndFilter(const CookedProjectEntry& entry) const;
    const AssetMetadata* SelectedMetadata(EditorContext& context) const;
    // PBR material editor shown in the details pane for a selected .material.yaml,
    // with save (reimport + recook) support.
    void DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path);
    // Pushes the in-progress material edits to the renderer as a live preview
    // (no disk write/recook) so the viewport updates while editing.
    void ApplyMaterialPreview(EditorContext& context);
    // Creates a new default material asset under `directory` and imports it.
    void CreateMaterialAsset(EditorContext& context, const std::filesystem::path& directory);
    void OpenSceneFile(EditorContext& context, const std::filesystem::path& path);
    void InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path);

    void RequestModal(Modal modal, const std::filesystem::path& target);

    ProjectBrowser m_Browser;
    ProjectSearchScope m_SearchScope = ProjectSearchScope::All;
    EditorFileType m_TypeFilter = EditorFileType::Unknown;
    std::filesystem::path m_SelectedFolder;
    AssetID m_SelectedAssetId;
    float m_IconSize = 72.0f;
    bool m_FocusSearchNextFrame = false;

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
