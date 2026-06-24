#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/Project/EditorAssetPreviewRenderer.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

struct ImVec2;

namespace Hockey {

struct AssetMetadata;

enum class ProjectSection {
    All,
    Models,
    Meshes,
    Materials,
    Prefabs,
    Scenes,
    Textures,
    Shaders,
};

// Project asset browser over raw source assets. The left side exposes fixed
// asset sections, while imported/importable files can select into Inspector.
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

    void EnsureSelectionState(EditorContext& context);
    void DrawToolbar(EditorContext& context);
    float SectionListWidth(EditorContext& context) const;
    void DrawSectionList(EditorContext& context);
    void DrawFolderContents(EditorContext& context);
    void DrawSearchResults(EditorContext& context);
    void DrawRawEntry(EditorContext& context, const ProjectEntry& entry);
    bool DrawProjectTileButton(EditorContext& context,
                               const ProjectEntry& entry,
                               const AssetMetadata* meta,
                               const ImVec2& size);
    std::uint64_t ThumbnailTextureId(EditorContext& context, const ProjectEntry& entry, const AssetMetadata* meta);
    void DrawRawContextMenu(EditorContext& context, const ProjectEntry& entry);
    void DrawStatusStrip(EditorContext& context);
    void DrawViewSizeSlider();
    void DrawModals(EditorContext& context);

    // Drag a prefab file (path payload) or an imported asset (asset-id payload)
    // out of the panel; double-click opens scenes / instances prefabs.
    void BeginRawDragSource(EditorContext& context, const ProjectEntry& entry);
    void HandleRawActivation(EditorContext& context, const ProjectEntry& entry);
    void SelectRawEntry(EditorContext& context, const ProjectEntry& entry);
    void ReimportAndRecook(EditorContext& context, const AssetMetadata& meta);
    void Recook(EditorContext& context, const AssetMetadata& meta);
    void ImportSelectedFile(EditorContext& context);
    void ImportExternalAsset(EditorContext& context, const std::filesystem::path& source);
    bool ImportAndCookRawAsset(EditorContext& context, const std::filesystem::path& rawPath);
    void ImportAllAndCook(EditorContext& context);
    void RecookDirty(EditorContext& context);
    void InvalidateAssetEvents(EditorContext& context);
    std::vector<ProjectEntry> SearchEntries(EditorContext& context, bool* truncated) const;
    bool MatchesSearchAndFilter(const ProjectEntry& entry, bool includeFolders) const;
    const AssetMetadata* SelectedMetadata(EditorContext& context) const;
    const AssetMetadata* MetadataForRawEntry(EditorContext& context, const ProjectEntry& entry);
    // Creates a new default material asset under `directory` and imports it.
    void CreateMaterialAsset(EditorContext& context, const std::filesystem::path& directory);
    void OpenSceneFile(EditorContext& context, const std::filesystem::path& path);
    void InstantiatePrefabFile(EditorContext& context, const std::filesystem::path& path);
    AssetType SelectedSectionAssetType() const;
    std::filesystem::path SelectedSectionRawFolder(EditorContext& context, AssetType fallbackType) const;

    void RequestModal(Modal modal, const std::filesystem::path& target);

    ProjectBrowser m_Browser;
    ProjectSection m_SelectedSection = ProjectSection::All;
    EditorFileType m_TypeFilter = EditorFileType::Unknown;
    std::filesystem::path m_SelectedFolder;
    AssetID m_SelectedAssetId;
    float m_IconSize = 72.0f;
    bool m_FocusSearchNextFrame = false;
    EditorAssetPreviewRenderer m_AssetPreviewRenderer;

    Modal m_Modal = Modal::None;
    bool m_OpenModalRequested = false;
    std::filesystem::path m_ModalTarget;
    char m_NameBuffer[256] = {};
    std::string m_Status;

};

} // namespace Hockey
