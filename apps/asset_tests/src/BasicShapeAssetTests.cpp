#include "Test.hpp"

#include <filesystem>
#include <memory>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Core/Paths.hpp"

using namespace Hockey;

namespace {

struct ExpectedBasicMesh {
    const char* label;
    const char* rawPath;
};

constexpr ExpectedBasicMesh kExpectedBasicMeshes[] = {
    {"cube", "data/raw/meshes/cube/cube_mesh.mesh.yaml"},
    {"sphere", "data/raw/meshes/sphere/sphere_mesh.mesh.yaml"},
    {"cylinder", "data/raw/meshes/cylinder/cylinder_mesh.mesh.yaml"},
    {"capsule", "data/raw/meshes/capsule/capsule_mesh.mesh.yaml"},
    {"torus", "data/raw/meshes/torus/torus_mesh.mesh.yaml"},
    {"plane", "data/raw/meshes/plane/plane_mesh.mesh.yaml"},
};

} // namespace

void RunBasicShapeAssetTests() {
    HockeyTest::BeginSuite("BasicShapeAssetTests");

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(Paths::Get().root))),
                 "asset manager initializes for project data");

    for (const ExpectedBasicMesh& expected : kExpectedBasicMeshes) {
        const AssetMetadata* meta = manager.Database().FindByRawPath(expected.rawPath);
        HK_CHECK_MSG(meta != nullptr, std::string(expected.label) + " mesh metadata exists");
        if (meta == nullptr) {
            continue;
        }

        HK_CHECK_MSG(meta->type == AssetType::Mesh, std::string(expected.label) + " is a mesh asset");
        HK_CHECK_MSG(meta->cooked, std::string(expected.label) + " mesh is cooked");
        HK_CHECK_MSG(!meta->missing, std::string(expected.label) + " mesh is not missing");
        HK_CHECK_MSG(std::filesystem::exists(Paths::Get().root / meta->rawPath),
                     std::string(expected.label) + " raw descriptor exists");
        HK_CHECK_MSG(std::filesystem::exists(Paths::Get().root / meta->cookedPath),
                     std::string(expected.label) + " cooked mesh exists");

        Result<std::shared_ptr<MeshAsset>> loaded = manager.Load<MeshAsset>(meta->id);
        HK_CHECK_MSG(static_cast<bool>(loaded), std::string(expected.label) + " cooked mesh loads");
        if (loaded) {
            HK_CHECK_MSG(!loaded.value->vertices.empty(), std::string(expected.label) + " has vertices");
            HK_CHECK_MSG(!loaded.value->indices.empty(), std::string(expected.label) + " has indices");
            HK_CHECK_MSG(loaded.value->indices.size() % 3 == 0,
                         std::string(expected.label) + " index buffer is triangles");
        }
    }

    manager.Shutdown();
}
