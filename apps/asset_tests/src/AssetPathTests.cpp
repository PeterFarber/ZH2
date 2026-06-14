#include "Test.hpp"

#include "Hockey/Assets/AssetPath.hpp"

using namespace Hockey;
namespace fs = std::filesystem;

void RunAssetPathTests() {
    HockeyTest::BeginSuite("AssetPathTests");

    const fs::path root = "/home/user/project";

    // Project-relative conversion from an absolute path under the root.
    const fs::path abs = "/home/user/project/data/raw/textures/ice.png";
    const fs::path rel = AssetPath::ToProjectRelative(root, abs);
    HK_CHECK_EQ(rel.generic_string(), std::string("data/raw/textures/ice.png"));

    // Round-trip back to absolute.
    const fs::path back = AssetPath::ToAbsolute(root, rel);
    HK_CHECK_EQ(back.generic_string(), std::string("/home/user/project/data/raw/textures/ice.png"));

    // Normalization collapses redundant separators / dot segments.
    const fs::path messy = AssetPath::Normalize("data/raw/../raw/./textures/ice.png");
    HK_CHECK_EQ(messy.generic_string(), std::string("data/raw/textures/ice.png"));

    // Windows-style backslash input normalizes to forward slashes via generic.
    const fs::path winStyle = AssetPath::Normalize(fs::path("data") / "raw" / "textures" / "ice.png");
    HK_CHECK_EQ(winStyle.generic_string(), std::string("data/raw/textures/ice.png"));

    // Root detection.
    HK_CHECK_MSG(AssetPath::IsUnderRoot(root, abs), "abs path is under root");
    HK_CHECK_MSG(!AssetPath::IsUnderRoot(root, "/etc/passwd"), "outside path not under root");

    // Sidecar generation + detection.
    const fs::path sidecar = AssetPath::MetadataSidecar("data/raw/textures/ice.png");
    HK_CHECK_EQ(sidecar.generic_string(), std::string("data/raw/textures/ice.png.meta.yaml"));
    HK_CHECK_MSG(AssetPath::IsMetadataSidecar(sidecar), "sidecar detected");
    HK_CHECK_MSG(!AssetPath::IsMetadataSidecar("data/raw/textures/ice.png"), "raw not a sidecar");

    // Cooked path generation per type.
    const AssetID id(42);
    const fs::path cookedTex = AssetPath::CookedPath("data/cooked", AssetType::Texture, id);
    HK_CHECK_EQ(cookedTex.generic_string(),
                std::string("data/cooked/assets/textures/42.tex.bin"));
    const fs::path cookedShader = AssetPath::CookedPath("data/cooked", AssetType::Shader, id);
    HK_CHECK_EQ(cookedShader.generic_string(), std::string("data/cooked/assets/shaders/42.spv"));
    const fs::path cookedMesh = AssetPath::CookedPath("data/cooked", AssetType::Mesh, id);
    HK_CHECK_EQ(cookedMesh.generic_string(), std::string("data/cooked/assets/meshes/42.mesh.bin"));
}
