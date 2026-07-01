#include "Test.hpp"

#include "Hockey/Animation/AnimationEvents.hpp"
#include "Hockey/Animation/AnimationGraph.hpp"
#include "Hockey/Animation/AnimationStateMachine.hpp"
#include "Hockey/Animation/AnimationSystem.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/Assets/Runtime/AnimationLoader.hpp"
#include "Hockey/Assets/Runtime/SkeletonLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

void WriteBinaryFixture(const fs::path& path, const std::vector<std::byte>& bytes) {
    FileSystem::CreateDirectories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

AnimationGraph MakeRequiredStateGraph(AssetID graphID, AssetID clipID) {
    AnimationGraph graph;
    graph.id = graphID;
    graph.name = "SkaterGraph";
    graph.initialState = "Idle";

    for (std::string_view stateName : AnimationGraph::RequiredHockeyStateNames()) {
        AnimationGraphState state;
        state.name = std::string(stateName);
        state.clipAsset = clipID;
        state.looping = state.name != "Shoot" && state.name != "Steal" && state.name != "GoalieSave";
        state.playbackSpeed = 1.0f;
        state.blendTimeSeconds = 0.15f;
        graph.states.push_back(std::move(state));
    }

    return graph;
}

void AddCookedSkeleton(AssetManager& manager, const fs::path& workspace, const SkeletonAsset& skeleton) {
    AssetMetadata metadata;
    metadata.id = skeleton.id;
    metadata.type = AssetType::Skeleton;
    metadata.name = skeleton.name;
    metadata.rawPath = "data/raw/animation/skeletons/test.skeleton.yaml";
    metadata.cookedPath = fs::path("data/cooked/assets/skeletons") / (skeleton.id.ToString() + ".skel.bin");
    metadata.cooked = true;
    metadata.dirty = false;
    WriteBinaryFixture(workspace / metadata.cookedPath, SkeletonLoader::EncodeCooked(skeleton, 11));
    manager.Database().AddOrUpdate(metadata);
}

void AddCookedAnimation(AssetManager& manager, const fs::path& workspace, const AnimationAsset& animation) {
    AssetMetadata metadata;
    metadata.id = animation.id;
    metadata.type = AssetType::Animation;
    metadata.name = animation.name;
    metadata.rawPath = "data/raw/animation/clips/test.anim.yaml";
    metadata.cookedPath = fs::path("data/cooked/assets/animations") / (animation.id.ToString() + ".anim.bin");
    metadata.dependencies.push_back(animation.skeletonAsset);
    metadata.cooked = true;
    metadata.dirty = false;
    WriteBinaryFixture(workspace / metadata.cookedPath, AnimationLoader::EncodeCooked(animation, 22));
    manager.Database().AddOrUpdate(metadata);
}

SkeletonAsset MakeSkeletonAsset(AssetID id) {
    SkeletonAsset skeleton;
    skeleton.id = id;
    skeleton.name = "OneBoneSkeleton";
    skeleton.bones.push_back({"Root", -1});
    return skeleton;
}

AnimationAsset MakeAnimationAsset(AssetID id, AssetID skeletonID) {
    AnimationAsset animation;
    animation.id = id;
    animation.name = "Idle";
    animation.skeletonAsset = skeletonID;
    animation.durationSeconds = 1.0f;
    animation.looping = true;

    AnimationBoneTrack track;
    track.boneIndex = 0;
    track.translations.push_back({0.0f, {0.0f, 0.0f, 0.0f}});
    track.translations.push_back({1.0f, {10.0f, 0.0f, 0.0f}});
    animation.tracks.push_back(track);
    animation.events.push_back({0.25f, "FootPlant"});
    return animation;
}

} // namespace

void RunAnimationGraphTests() {
    HockeyTest::BeginSuite("AnimationGraph");

    const AssetID graphID{7001};
    const AssetID clipID{8001};
    AnimationGraph graph = MakeRequiredStateGraph(graphID, clipID);
    HK_CHECK(graph.HasRequiredHockeyStates());

    AnimationGraphState* idle = graph.FindState("Idle");
    HK_CHECK(idle != nullptr);
    if (idle != nullptr) {
        AnimationGraphTransition shootTransition;
        shootTransition.targetState = "Shoot";
        shootTransition.conditions.push_back(AnimationTransitionCondition::BoolEquals("shooting", true));
        idle->transitions.push_back(shootTransition);

        AnimationGraphTransition skateTransition;
        skateTransition.targetState = "SkateForward";
        skateTransition.conditions.push_back(AnimationTransitionCondition::FloatGreaterOrEqual("speed", 1.0f));
        idle->transitions.push_back(skateTransition);
    }
    HK_CHECK(graph.Validate());

    AnimatorComponent animator;
    animator.currentState = "Idle";
    animator.playbackTimeSeconds = 0.4f;
    AnimationParameterComponent parameters;
    parameters.speed = 4.0f;
    parameters.shooting = true;

    const AnimationStateTransitionResult transition =
        AnimationStateMachine::EvaluateTransitions(graph, parameters, animator);
    HK_CHECK(transition.transitioned);
    HK_CHECK_EQ(animator.currentState, std::string("Shoot"));
    HK_CHECK_NEAR(animator.playbackTimeSeconds, 0.0f, 0.0001f);

    AnimationClip eventClip;
    eventClip.id = AssetID{8100};
    eventClip.durationSeconds = 1.0f;
    eventClip.looping = true;
    eventClip.tracks.push_back({0});
    eventClip.tracks[0].translations.push_back({0.0f, {0.0f, 0.0f, 0.0f}});
    eventClip.events.push_back({0.05f, "WrapEvent"});
    eventClip.events.push_back({0.75f, "LateEvent"});

    const std::vector<AnimationEvent> events =
        CollectAnimationEvents(eventClip, "Idle", 0.60f, 1.10f, true);
    HK_CHECK_EQ(events.size(), 2u);
    if (events.size() == 2u) {
        HK_CHECK_EQ(events[0].name, std::string("LateEvent"));
        HK_CHECK_EQ(events[1].name, std::string("WrapEvent"));
    }

    Scene disabledScene("DisabledAnimation");
    Entity disabledEntity = disabledScene.CreateEntity("Player");
    disabledEntity.AddComponent<AnimatorComponent>();
    disabledEntity.AddComponent<AnimationParameterComponent>();
    disabledEntity.AddComponent<SkinnedMeshRendererComponent>();
    AnimationPosePaletteComponent disabledPalette;
    disabledPalette.valid = true;
    disabledPalette.skinningMatrices.push_back(glm::mat4{2.0f});
    disabledEntity.AddComponent<AnimationPosePaletteComponent>(disabledPalette);

    const fs::path disabledWorkspace = Paths::TempFile("animation_disabled_ws");
    FileSystem::Remove(disabledWorkspace);
    AssetManager disabledManager;
    HK_CHECK(static_cast<bool>(disabledManager.Init(AssetManager::DefaultCreateInfo(disabledWorkspace))));
    AnimationSystem disabledSystem(AnimationSettings{.enabled = false});
    HK_CHECK(disabledSystem.Update(disabledScene, disabledManager, 0.5f));
    const AnimationPosePaletteComponent& stillValid = disabledEntity.GetComponent<AnimationPosePaletteComponent>();
    HK_CHECK(stillValid.valid);
    HK_CHECK_EQ(stillValid.skinningMatrices.size(), 1u);
    disabledManager.Shutdown();
    FileSystem::Remove(disabledWorkspace);

    const fs::path workspace = Paths::TempFile("animation_system_ws");
    FileSystem::Remove(workspace);
    AssetManager manager;
    HK_CHECK(static_cast<bool>(manager.Init(AssetManager::DefaultCreateInfo(workspace))));

    const AssetID skeletonID{9001};
    const AssetID animationID{9002};
    AddCookedSkeleton(manager, workspace, MakeSkeletonAsset(skeletonID));
    AddCookedAnimation(manager, workspace, MakeAnimationAsset(animationID, skeletonID));

    AnimationGraph runtimeGraph = MakeRequiredStateGraph(graphID, animationID);
    runtimeGraph.initialState = "Idle";
    AnimationSystem system;
    system.RegisterGraph(runtimeGraph);

    Scene scene("AnimationSystem");
    Entity entity = scene.CreateEntity("AnimatedPlayer");
    AnimatorComponent runtimeAnimator;
    runtimeAnimator.animationGraphAsset = graphID.Value();
    entity.AddComponent<AnimatorComponent>(runtimeAnimator);
    SkinnedMeshRendererComponent skinned;
    skinned.skeletonAsset = skeletonID.Value();
    entity.AddComponent<SkinnedMeshRendererComponent>(skinned);

    HK_CHECK(system.Update(scene, manager, 0.5f));
    HK_CHECK(entity.HasComponent<AnimationPosePaletteComponent>());
    if (entity.HasComponent<AnimationPosePaletteComponent>()) {
        const AnimationPosePaletteComponent& palette = entity.GetComponent<AnimationPosePaletteComponent>();
        HK_CHECK(palette.valid);
        HK_CHECK_EQ(palette.skinningMatrices.size(), 1u);
        if (!palette.skinningMatrices.empty()) {
            HK_CHECK_NEAR(palette.skinningMatrices[0][3].x, 5.0f, 0.0001f);
        }
    }
    HK_CHECK_NEAR(entity.GetComponent<AnimatorComponent>().playbackTimeSeconds, 0.5f, 0.0001f);
    const std::vector<AnimationEvent> emitted = system.DrainEvents();
    HK_CHECK_EQ(emitted.size(), 1u);
    if (!emitted.empty()) {
        HK_CHECK_EQ(emitted.front().name, std::string("FootPlant"));
    }
    HK_CHECK_EQ(system.GetStats().updatedPaletteCount, 1u);
    manager.Shutdown();
    FileSystem::Remove(workspace);

    const fs::path missingWorkspace = Paths::TempFile("animation_missing_ws");
    FileSystem::Remove(missingWorkspace);
    AssetManager missingManager;
    HK_CHECK(static_cast<bool>(missingManager.Init(AssetManager::DefaultCreateInfo(missingWorkspace))));

    AnimationSystem missingSystem;
    AnimationGraph missingClipGraph = MakeRequiredStateGraph(AssetID{7010}, AssetID{999999});
    missingSystem.RegisterGraph(missingClipGraph);

    Scene missingScene("MissingAnimationAssets");
    Entity missingGraphEntity = missingScene.CreateEntity("MissingGraph");
    AnimatorComponent missingGraphAnimator;
    missingGraphAnimator.animationGraphAsset = 1234567;
    missingGraphEntity.AddComponent<AnimatorComponent>(missingGraphAnimator);
    missingGraphEntity.AddComponent<AnimationParameterComponent>();
    missingGraphEntity.AddComponent<SkinnedMeshRendererComponent>();

    Entity missingClipEntity = missingScene.CreateEntity("MissingClip");
    AnimatorComponent missingClipAnimator;
    missingClipAnimator.animationGraphAsset = 7010;
    missingClipEntity.AddComponent<AnimatorComponent>(missingClipAnimator);
    missingClipEntity.AddComponent<AnimationParameterComponent>();
    SkinnedMeshRendererComponent missingSkinned;
    missingSkinned.skeletonAsset = skeletonID.Value();
    missingClipEntity.AddComponent<SkinnedMeshRendererComponent>(missingSkinned);

    HK_CHECK(missingSystem.Update(missingScene, missingManager, 0.1f));
    HK_CHECK_EQ(missingSystem.GetStats().missingGraphCount, 1u);
    HK_CHECK_EQ(missingSystem.GetStats().missingSkeletonCount, 1u);
    HK_CHECK_EQ(missingSystem.GetStats().missingClipCount, 0u);
    missingManager.Shutdown();
    FileSystem::Remove(missingWorkspace);
}
