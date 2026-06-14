#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Decodes a raw texture, generates a mip chain, and writes the cooked binary
// (.tex.bin) under data/cooked/assets/textures. The cooker interface allows a
// future upgrade to KTX2/Basis compression without changing call sites.
class TextureCooker : public Cooker {
public:
    AssetType GetAssetType() const override { return AssetType::Texture; }
    std::string Version() const override { return "TextureCooker_v1"; }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
