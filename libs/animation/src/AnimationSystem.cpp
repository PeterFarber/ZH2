#include "Hockey/Animation/AnimationSystem.hpp"

#include "Hockey/Animation/AnimationSampler.hpp"
#include "Hockey/Animation/AnimationStateMachine.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/AnimationAsset.hpp"
#include "Hockey/Assets/Assets/SkeletonAsset.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"

#include <algorithm>
#include <memory>

namespace Hockey {
namespace {

Skeleton ToSkeleton(const SkeletonAsset& asset) {
    Skeleton skeleton;
    skeleton.id = asset.id;
    skeleton.name = asset.name;
    skeleton.bones.reserve(asset.bones.size());
    for (const SkeletonAssetBone& source : asset.bones) {
        Bone bone;
        bone.name = source.name;
        bone.parentIndex = source.parentIndex;
        bone.inverseBindPose = source.inverseBindPose;
        bone.localBindTransform = source.localBindTransform;
        skeleton.bones.push_back(std::move(bone));
    }
    return skeleton;
}

AnimationClip ToClip(const AnimationAsset& asset, bool stateLooping) {
    AnimationClip clip;
    clip.id = asset.id;
    clip.name = asset.name;
    clip.durationSeconds = asset.durationSeconds;
    clip.looping = stateLooping;
    clip.tracks.reserve(asset.tracks.size());
    for (const AnimationBoneTrack& source : asset.tracks) {
        BoneTrack track;
        track.boneIndex = source.boneIndex;
        track.translations.reserve(source.translations.size());
        for (const AnimationAssetTranslationKey& key : source.translations) {
            track.translations.push_back({key.timeSeconds, key.value});
        }
        track.rotations.reserve(source.rotations.size());
        for (const AnimationAssetRotationKey& key : source.rotations) {
            track.rotations.push_back({key.timeSeconds, key.value});
        }
        track.scales.reserve(source.scales.size());
        for (const AnimationAssetScaleKey& key : source.scales) {
            track.scales.push_back({key.timeSeconds, key.value});
        }
        clip.tracks.push_back(std::move(track));
    }

    clip.events.reserve(asset.events.size());
    for (const AnimationAssetEvent& event : asset.events) {
        clip.events.push_back({event.timeSeconds, event.name});
    }
    return clip;
}

float NormalizePlaybackTime(const AnimationClip& clip, float timeSeconds) {
    if (clip.durationSeconds <= 0.0f) {
        return 0.0f;
    }
    if (clip.looping) {
        float wrapped = std::fmod(timeSeconds, clip.durationSeconds);
        if (wrapped < 0.0f) {
            wrapped += clip.durationSeconds;
        }
        return wrapped;
    }
    return std::clamp(timeSeconds, 0.0f, clip.durationSeconds);
}

void MarkPaletteInvalid(entt::registry& registry, entt::entity entity) {
    AnimationPosePaletteComponent& palette =
        registry.all_of<AnimationPosePaletteComponent>(entity)
            ? registry.get<AnimationPosePaletteComponent>(entity)
            : registry.emplace<AnimationPosePaletteComponent>(entity);
    palette.valid = false;
    palette.skinningMatrices.clear();
}

} // namespace

AnimationSystem::AnimationSystem(AnimationSettings settings) : m_Settings(settings) {}

const AnimationSettings& AnimationSystem::GetSettings() const {
    return m_Settings;
}

void AnimationSystem::SetSettings(AnimationSettings settings) {
    m_Settings = settings;
}

Status AnimationSystem::RegisterGraph(AnimationGraph graph) {
    if (const Status status = graph.Validate(); !status) {
        return status;
    }
    m_Graphs[graph.id] = std::move(graph);
    return Status::Ok();
}

void AnimationSystem::ClearGraphs() {
    m_Graphs.clear();
}

Status AnimationSystem::Update(Scene& scene, AssetManager& assetManager, float dtSeconds) {
    m_Stats = {};
    if (!m_Settings.enabled) {
        return Status::Ok();
    }

    const float clampedDelta = std::max(0.0f, dtSeconds);
    entt::registry& registry = scene.Registry();
    auto view = registry.view<AnimatorComponent, SkinnedMeshRendererComponent>();
    for (entt::entity entity : view) {
        AnimatorComponent& animator = view.get<AnimatorComponent>(entity);
        const SkinnedMeshRendererComponent& skinned = view.get<SkinnedMeshRendererComponent>(entity);
        const AnimationParameterComponent defaultParameters;
        const AnimationParameterComponent* authoredParameters = registry.try_get<AnimationParameterComponent>(entity);
        const AnimationParameterComponent& parameters =
            authoredParameters != nullptr ? *authoredParameters : defaultParameters;

        const AnimationGraph* graph = FindGraph(AssetID{animator.animationGraphAsset});
        if (graph == nullptr) {
            ++m_Stats.missingGraphCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        AnimationStateMachine::EvaluateTransitions(*graph, parameters, animator);
        const AnimationGraphState* state = AnimationStateMachine::ResolveCurrentState(*graph, animator);
        if (state == nullptr || !state->clipAsset.IsValid()) {
            ++m_Stats.missingClipCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        const AssetID skeletonID{skinned.skeletonAsset};
        if (!skeletonID.IsValid()) {
            ++m_Stats.missingSkeletonCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        Result<std::shared_ptr<SkeletonAsset>> skeletonAsset = assetManager.Load<SkeletonAsset>(skeletonID);
        if (!skeletonAsset) {
            ++m_Stats.missingSkeletonCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        Result<std::shared_ptr<AnimationAsset>> animationAsset = assetManager.Load<AnimationAsset>(state->clipAsset);
        if (!animationAsset) {
            ++m_Stats.missingClipCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        Skeleton skeleton = ToSkeleton(*skeletonAsset.value);
        if (const Status skeletonStatus = skeleton.Validate(m_Settings.maxBones); !skeletonStatus) {
            ++m_Stats.invalidSkeletonCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        AnimationClip clip = ToClip(*animationAsset.value, state->looping);
        if (const Status clipStatus = clip.Validate(skeleton); !clipStatus) {
            ++m_Stats.invalidClipCount;
            MarkPaletteInvalid(registry, entity);
            continue;
        }

        const float previousTime = animator.playbackTimeSeconds;
        const float stateSpeed = std::max(0.0f, state->playbackSpeed);
        const float playbackSpeed = std::max(0.0f, animator.playbackSpeed) * stateSpeed;
        const float currentTime = previousTime + (animator.playing ? clampedDelta * playbackSpeed : 0.0f);

        AnimationPose pose;
        AnimationSampler::Sample(clip, skeleton, currentTime, pose);
        AnimationSampler::BuildModelSpacePose(skeleton, pose);
        AnimationSampler::BuildSkinningMatrices(skeleton, pose);

        AnimationPosePaletteComponent& palette =
            registry.all_of<AnimationPosePaletteComponent>(entity)
                ? registry.get<AnimationPosePaletteComponent>(entity)
                : registry.emplace<AnimationPosePaletteComponent>(entity);
        palette.skinningMatrices = std::move(pose.skinningMatrices);
        palette.valid = true;
        ++m_Stats.updatedPaletteCount;

        if (m_Settings.enableAnimationEvents) {
            std::vector<AnimationEvent> emitted =
                CollectAnimationEvents(clip, state->name, previousTime, currentTime, clip.looping);
            m_Stats.emittedEventCount += emitted.size();
            m_Events.insert(m_Events.end(), emitted.begin(), emitted.end());
        }

        animator.playbackTimeSeconds = NormalizePlaybackTime(clip, currentTime);
    }

    return Status::Ok();
}

const AnimationSystemStats& AnimationSystem::GetStats() const {
    return m_Stats;
}

std::vector<AnimationEvent> AnimationSystem::DrainEvents() {
    std::vector<AnimationEvent> drained = std::move(m_Events);
    m_Events.clear();
    return drained;
}

const AnimationGraph* AnimationSystem::FindGraph(AssetID id) const {
    const auto it = m_Graphs.find(id);
    return it == m_Graphs.end() ? nullptr : &it->second;
}

} // namespace Hockey
