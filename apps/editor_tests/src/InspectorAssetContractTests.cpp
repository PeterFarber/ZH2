#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunInspectorAssetContractTests() {
    HockeyTest::BeginSuite("InspectorAssetContractTests");

    const std::string inspectorHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/InspectorPanel.hpp");
    const std::string inspectorSource = ReadProjectFile("libs/editor/src/Panels/InspectorPanel.cpp");
    const std::string assetHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/Inspector/AssetInspector.hpp");
    const std::string assetSource = ReadProjectFile("libs/editor/src/Inspector/AssetInspector.cpp");

    HK_CHECK_MSG(Contains(inspectorHeader, "AssetInspector"), "Inspector panel owns an asset inspector");
    HK_CHECK_MSG(Contains(inspectorSource, "context.SyncAssetSelectionWithEntitySelection"),
                 "Inspector reconciles asset selection with entity selection");
    HK_CHECK_MSG(Contains(inspectorSource, "context.SelectedAsset"), "Inspector checks selected cooked asset id");
    HK_CHECK_MSG(Contains(inspectorSource, "m_AssetInspector.Draw"), "Inspector delegates cooked assets");
    HK_CHECK_MSG(Contains(inspectorHeader, "m_InspectorLocked"), "Inspector panel stores lock state");
    HK_CHECK_MSG(Contains(inspectorHeader, "m_LockedAssetId"), "Inspector panel can lock to an asset selection");
    HK_CHECK_MSG(Contains(inspectorHeader, "m_LockedEntityId"), "Inspector panel can lock to an entity selection");
    HK_CHECK_MSG(Contains(inspectorSource, "EditorIconToggleButton(EditorIcon::Locked"),
                 "Inspector exposes a lock toggle button");
    HK_CHECK_MSG(Contains(inspectorSource, "m_InspectorLocked") &&
                     Contains(inspectorSource, "m_AssetInspector.Draw(context, m_LockedAssetId)"),
                 "Inspector keeps drawing the locked asset while Project selection changes");

    HK_CHECK_MSG(Contains(assetHeader, "class AssetInspector"), "Asset inspector type exists");
    HK_CHECK_MSG(Contains(assetHeader, "DrawTexturePreview"), "Asset inspector has texture preview UI");
    HK_CHECK_MSG(Contains(assetHeader, "DrawMaterialPreview"), "Asset inspector has material sphere preview UI");
    HK_CHECK_MSG(Contains(assetHeader, "EditorAssetPreviewRenderer"), "Asset inspector owns shared preview renderer");
    HK_CHECK_MSG(Contains(assetSource, "DrawMaterialEditor"), "Asset inspector edits material assets");
    HK_CHECK_MSG(Contains(assetSource, "IsInspectableAsset") && !Contains(assetSource, "IsVisibleCookedAsset"),
                 "Asset inspector accepts imported raw assets without requiring cooked output");
    HK_CHECK_MSG(Contains(assetSource, "DrawTexturePreview(context, *meta)"),
                 "Texture assets draw an Inspector preview section");
    HK_CHECK_MSG(Contains(assetSource, "context.imguiBridge->TextureIdForAsset(meta.id.Value())"),
                 "Texture Inspector preview uses ImGui texture ids for asset thumbnails");
    HK_CHECK_MSG(Contains(assetSource, "Load<TextureAsset>(meta.id)") && Contains(assetSource, "texture.value->width") &&
                     Contains(assetSource, "texture.value->height"),
                 "Texture Inspector preview uses cooked texture dimensions when available");
    HK_CHECK_MSG(Contains(assetSource, "DrawMaterialPreview(context)") &&
                     Contains(assetSource, "m_AssetPreviewRenderer.MaterialPreviewTextureId"),
                 "Material assets draw an Inspector sphere preview via the shared helper");
    HK_CHECK_MSG(Contains(assetSource, "ApplyMaterialPreview(context)") &&
                     assetSource.find("ApplyMaterialPreview(context)") < assetSource.find("DrawMaterialPreview(context)"),
                 "Material field edits apply live preview before drawing the material sphere");
    HK_CHECK_MSG(Contains(assetSource, "MaterialSerializer::SaveFile"),
                 "Material asset edits save the raw authoring source");
    HK_CHECK_MSG(Contains(assetSource, "CookAllDirty()"), "Material asset edits recook dirty assets");
    HK_CHECK_MSG(Contains(assetSource, "requestedOpenScene"), "Scene assets can be opened from Inspector");
}
