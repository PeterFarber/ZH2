#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <glm/glm.hpp>

namespace Hockey {

// Triangle-mesh collision geometry resolved from a mesh asset. Positions are in
// the mesh's local space; indices form a triangle list (3 indices per triangle).
struct PhysicsMeshData {
    std::vector<glm::vec3> vertices;
    std::vector<std::uint32_t> indices;
};

// Resolves a mesh asset id (the raw uint64 stored in MeshColliderComponent) into
// collision geometry. Returns false when the id is unknown/unavailable.
using PhysicsMeshProvider = std::function<bool(std::uint64_t meshAsset, PhysicsMeshData& out)>;

// ---------------------------------------------------------------------------
// Process-wide provider seam. `hockey_physics` must not depend on
// `hockey_assets` (see the dependency graph), so it cannot load mesh data on its
// own. The app — which does have asset access — installs a provider that reads
// the AssetManager, and the physics shape factory queries it when building a
// MeshColliderComponent. This keeps mesh-collider support working without
// violating the architecture boundary.
// ---------------------------------------------------------------------------
class PhysicsMeshRegistry {
public:
    static PhysicsMeshRegistry& Get();

    void SetProvider(PhysicsMeshProvider provider);
    bool HasProvider() const;

    // Resolves geometry for `meshAsset`. Returns false (and leaves `out`
    // untouched) when no provider is installed or the provider reports failure.
    bool Resolve(std::uint64_t meshAsset, PhysicsMeshData& out) const;

    void Clear();

private:
    PhysicsMeshProvider m_Provider;
};

} // namespace Hockey
