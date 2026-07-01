#include "Test.hpp"

#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/CookedFormat.hpp"
#include "Hockey/Assets/Runtime/AnimationLoader.hpp"
#include "Hockey/Assets/Runtime/MeshLoader.hpp"
#include "Hockey/Assets/Runtime/SkeletonLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"

#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <fstream>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

struct MeshVertexV1 {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    glm::vec2 uv0{0.0f};
};

std::vector<std::byte> EncodeVersion1StaticMesh(AssetID id) {
    std::vector<std::byte> buffer;
    BinaryWriter writer(buffer);
    writer.WriteHeader(CookedFormat::MakeHeader(AssetType::Mesh, id, 123, 1));

    MeshVertexV1 vertex;
    vertex.position = {1.0f, 2.0f, 3.0f};
    writer.Write<uint32_t>(1);
    writer.WriteBytes(&vertex, sizeof(vertex));

    const uint32_t indices[]{0};
    writer.Write<uint32_t>(1);
    writer.WriteBytes(indices, sizeof(indices));

    writer.Write<uint32_t>(0);
    writer.Write<float>(-1.0f);
    writer.Write<float>(-2.0f);
    writer.Write<float>(-3.0f);
    writer.Write<float>(1.0f);
    writer.Write<float>(2.0f);
    writer.Write<float>(3.0f);
    return buffer;
}

void WriteBinaryFixture(const fs::path& path, const std::vector<std::byte>& bytes) {
    FileSystem::CreateDirectories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

} // namespace

void RunAnimationAssetTests() {
    HockeyTest::BeginSuite("AnimationAssetTests");

    HK_CHECK_EQ(AssetTypeToString(AssetType::Skeleton), std::string("Skeleton"));
    HK_CHECK_EQ(AssetTypeFromString("Skeleton"), AssetType::Skeleton);
    HK_CHECK_EQ(AssetPath::CookedSubdirectory(AssetType::Skeleton), std::string("skeletons"));
    HK_CHECK_EQ(AssetPath::CookedExtension(AssetType::Skeleton), std::string(".skel.bin"));

    const glm::u16vec4 zeroJoints{0, 0, 0, 0};
    const glm::vec4 zeroWeights{0.0f, 0.0f, 0.0f, 0.0f};

    MeshVertex defaultVertex;
    HK_CHECK_EQ(defaultVertex.jointIndices, zeroJoints);
    HK_CHECK_EQ(defaultVertex.jointWeights, zeroWeights);

    MeshAsset skinnedMesh;
    skinnedMesh.id = AssetID{77};
    MeshVertex skinnedVertex;
    skinnedVertex.position = {1.0f, 0.0f, 0.0f};
    skinnedVertex.jointIndices = {1, 2, 0, 0};
    skinnedVertex.jointWeights = {0.75f, 0.25f, 0.0f, 0.0f};
    skinnedMesh.vertices.push_back(skinnedVertex);
    skinnedMesh.indices.push_back(0);

    const std::vector<std::byte> encodedMesh = MeshLoader::Encode(skinnedMesh, 99);
    const Result<MeshAsset> decodedMesh = MeshLoader::Decode(encodedMesh.data(), encodedMesh.size());
    HK_CHECK_MSG(static_cast<bool>(decodedMesh), "decode skinned mesh v2");
    if (decodedMesh) {
        const glm::u16vec4 expectedJoints{1, 2, 0, 0};
        HK_CHECK_EQ(decodedMesh.value.vertices[0].jointIndices, expectedJoints);
        HK_CHECK_NEAR(decodedMesh.value.vertices[0].jointWeights.x, 0.75f, 0.0001f);
        HK_CHECK_NEAR(decodedMesh.value.vertices[0].jointWeights.y, 0.25f, 0.0001f);
    }

    const std::vector<std::byte> encodedV1 = EncodeVersion1StaticMesh(AssetID{88});
    const Result<MeshAsset> decodedV1 = MeshLoader::Decode(encodedV1.data(), encodedV1.size());
    HK_CHECK_MSG(static_cast<bool>(decodedV1), "decode legacy static mesh v1");
    if (decodedV1) {
        HK_CHECK_EQ(decodedV1.value.vertices.size(), 1u);
        HK_CHECK_EQ(decodedV1.value.vertices[0].jointIndices, zeroJoints);
        HK_CHECK_EQ(decodedV1.value.vertices[0].jointWeights, zeroWeights);
    }

    SkeletonAsset skeleton;
    skeleton.id = AssetID{101};
    skeleton.name = "SkaterSkeleton";
    skeleton.bones.push_back({"Root", -1});
    skeleton.bones.push_back({"StickHand", 0});
    const std::vector<std::byte> encodedSkeleton = SkeletonLoader::EncodeCooked(skeleton, 55);
    const Result<SkeletonAsset> decodedSkeleton = SkeletonLoader::DecodeCooked(encodedSkeleton.data(), encodedSkeleton.size());
    HK_CHECK_MSG(static_cast<bool>(decodedSkeleton), "decode skeleton asset");
    if (decodedSkeleton) {
        HK_CHECK_EQ(decodedSkeleton.value.id, skeleton.id);
        HK_CHECK_EQ(decodedSkeleton.value.bones.size(), 2u);
        HK_CHECK_EQ(decodedSkeleton.value.bones[1].parentIndex, 0);
    }

    AnimationAsset animation;
    animation.id = AssetID{202};
    animation.name = "SkateForward";
    animation.skeletonAsset = skeleton.id;
    animation.durationSeconds = 1.0f;
    animation.looping = true;
    AnimationBoneTrack track;
    track.boneIndex = 1;
    track.translations.push_back({0.0f, {0.0f, 0.0f, 0.0f}});
    track.translations.push_back({1.0f, {1.0f, 0.0f, 0.0f}});
    animation.tracks.push_back(track);
    animation.events.push_back({0.5f, "SkatePlant"});

    const std::vector<std::byte> encodedAnimation = AnimationLoader::EncodeCooked(animation, 66);
    const Result<AnimationAsset> decodedAnimation =
        AnimationLoader::DecodeCooked(encodedAnimation.data(), encodedAnimation.size());
    HK_CHECK_MSG(static_cast<bool>(decodedAnimation), "decode animation asset");
    if (decodedAnimation) {
        HK_CHECK_EQ(decodedAnimation.value.id, animation.id);
        HK_CHECK_EQ(decodedAnimation.value.skeletonAsset, skeleton.id);
        HK_CHECK_EQ(decodedAnimation.value.tracks.size(), 1u);
        HK_CHECK_EQ(decodedAnimation.value.events.size(), 1u);
    }

    const fs::path workspace = Paths::TempFile("animation_asset_ws");
    FileSystem::Remove(workspace);

    AssetManager manager;
    HK_CHECK_MSG(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))), "manager init");

    AssetMetadata skeletonMeta;
    skeletonMeta.id = skeleton.id;
    skeletonMeta.type = AssetType::Skeleton;
    skeletonMeta.name = skeleton.name;
    skeletonMeta.rawPath = "data/raw/animation/skeletons/skater.skeleton.yaml";
    skeletonMeta.cookedPath = fs::path("data/cooked/assets/skeletons") / (skeleton.id.ToString() + ".skel.bin");
    skeletonMeta.cooked = true;
    skeletonMeta.dirty = false;
    WriteBinaryFixture(workspace / skeletonMeta.cookedPath, encodedSkeleton);
    manager.Database().AddOrUpdate(skeletonMeta);

    AssetMetadata animationMeta;
    animationMeta.id = animation.id;
    animationMeta.type = AssetType::Animation;
    animationMeta.name = animation.name;
    animationMeta.rawPath = "data/raw/animation/clips/skate_forward.anim.yaml";
    animationMeta.cookedPath = fs::path("data/cooked/assets/animations") / (animation.id.ToString() + ".anim.bin");
    animationMeta.cooked = true;
    animationMeta.dirty = false;
    animationMeta.dependencies.push_back(skeleton.id);
    WriteBinaryFixture(workspace / animationMeta.cookedPath, encodedAnimation);
    manager.Database().AddOrUpdate(animationMeta);

    const Result<std::shared_ptr<SkeletonAsset>> loadedSkeleton = manager.Load<SkeletonAsset>(skeleton.id);
    HK_CHECK_MSG(static_cast<bool>(loadedSkeleton), "load skeleton through AssetManager");
    if (loadedSkeleton) {
        HK_CHECK_EQ(loadedSkeleton.value->bones.size(), 2u);
    }

    const Result<std::shared_ptr<AnimationAsset>> loadedAnimation = manager.Load<AnimationAsset>(animation.id);
    HK_CHECK_MSG(static_cast<bool>(loadedAnimation), "load animation through AssetManager");
    if (loadedAnimation) {
        HK_CHECK_EQ(loadedAnimation.value->skeletonAsset, skeleton.id);
        HK_CHECK_EQ(loadedAnimation.value->tracks.size(), 1u);
    }

    manager.Shutdown();
    FileSystem::Remove(workspace);
}
