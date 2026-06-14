#pragma once
#include "Hockey/Assets/Cooker.hpp"

namespace Hockey {

// Cooks a glTF-generated mesh sub-asset ("<model>#mesh<i>") into a cooked
// .mesh.bin: re-parses the source glTF, resolves submesh materials to AssetIDs,
// runs meshoptimizer (dedup + vertex cache + vertex fetch), and writes the
// binary mesh. Reports the resolved material AssetIDs as dependencies.
class MeshCooker : public Cooker {
public:
    AssetType GetAssetType() const override {
        return AssetType::Mesh;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    CookResult Cook(const CookContext& context) override;
};

} // namespace Hockey
