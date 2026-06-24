#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "Hockey/Renderer/RenderHandles.hpp"

namespace Hockey {

class EditorContext;

class EditorAssetPreviewRenderer {
public:
    std::uint64_t MaterialPreviewTextureId(EditorContext& context,
                                           std::uint64_t materialAssetId,
                                           std::uint32_t previewSize);
    void Clear();

private:
    struct MaterialPreviewKey {
        std::uint64_t materialAssetId = 0;
        std::uint32_t previewSize = 0;

        bool operator==(const MaterialPreviewKey& other) const = default;
    };

    struct MaterialPreviewKeyHash {
        std::size_t operator()(const MaterialPreviewKey& key) const noexcept;
    };

    std::unordered_map<MaterialPreviewKey, TextureHandle, MaterialPreviewKeyHash> m_MaterialPreviewTargets;
};

} // namespace Hockey
