#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/Project/ProjectBrowser.hpp"

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

// Project asset browser over cooked assets. The left side exposes fixed asset
// sections, while selected asset details/editing live in the Inspector panel.
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
    void DrawCookedEntry(EditorContext& context, const CookedProjectEntry& entry);
    void DrawCookedContextMenu(EditorContext& context, const CookedProjectEntry& entry);
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

    Modal m_Modal = Modal::None;
    bool m_OpenModalRequested = false;
    std::filesystem::path m_ModalTarget;
    char m_NameBuffer[256] = {};
    std::string m_Status;

};

} // namespace Hockey
