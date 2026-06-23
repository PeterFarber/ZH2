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

    HK_CHECK_MSG(Contains(assetHeader, "class AssetInspector"), "Asset inspector type exists");
    HK_CHECK_MSG(Contains(assetSource, "DrawMaterialEditor"), "Asset inspector edits material assets");
    HK_CHECK_MSG(Contains(assetSource, "MaterialSerializer::SaveFile"),
                 "Material asset edits save the raw authoring source");
    HK_CHECK_MSG(Contains(assetSource, "CookAllDirty()"), "Material asset edits recook dirty assets");
    HK_CHECK_MSG(Contains(assetSource, "requestedOpenScene"), "Scene assets can be opened from Inspector");
}
