#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"

namespace Hockey {

class EditorContext;
struct AssetMetadata;

class AssetInspector {
public:
    void Draw(EditorContext& context, AssetID assetId);

private:
    void DrawMetadata(EditorContext& context, const AssetMetadata& meta);
    void DrawMaterialEditor(EditorContext& context, const std::filesystem::path& path);
    void ApplyMaterialPreview(EditorContext& context);
    void ReimportAndRecook(EditorContext& context, const AssetMetadata& meta);
    void Recook(EditorContext& context, const AssetMetadata& meta);
    void InvalidateAssetEvents(EditorContext& context);

    MaterialSource m_EditMaterial;
    std::filesystem::path m_EditMaterialPath;
    std::uint64_t m_EditMaterialId = 0;
    bool m_MaterialPreviewActive = false;
    std::string m_Status;
};

} // namespace Hockey
