#include "Hockey/Animation/AnimationSampler.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace Hockey {
namespace {

float NormalizeSampleTime(const AnimationClip& clip, float timeSeconds) {
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

LocalBoneTransform BindTransformFromMatrix(const glm::mat4& matrix) {
    LocalBoneTransform transform;
    transform.translation = glm::vec3{matrix[3]};

    const glm::vec3 xAxis{matrix[0]};
    const glm::vec3 yAxis{matrix[1]};
    const glm::vec3 zAxis{matrix[2]};
    transform.scale = {glm::length(xAxis), glm::length(yAxis), glm::length(zAxis)};

    glm::mat3 rotationMatrix{1.0f};
    if (transform.scale.x > 0.0f) {
        rotationMatrix[0] = xAxis / transform.scale.x;
    }
    if (transform.scale.y > 0.0f) {
        rotationMatrix[1] = yAxis / transform.scale.y;
    }
    if (transform.scale.z > 0.0f) {
        rotationMatrix[2] = zAxis / transform.scale.z;
    }
    transform.rotation = glm::normalize(glm::quat_cast(rotationMatrix));
    return transform;
}

glm::mat4 ComposeLocalMatrix(const LocalBoneTransform& transform) {
    const glm::mat4 translation = glm::translate(glm::mat4{1.0f}, transform.translation);
    const glm::mat4 rotation = glm::mat4_cast(glm::normalize(transform.rotation));
    const glm::mat4 scale = glm::scale(glm::mat4{1.0f}, transform.scale);
    return translation * rotation * scale;
}

void SetBindPoseLocals(const Skeleton& skeleton, AnimationPose& pose) {
    pose.localTransforms.resize(skeleton.bones.size());
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        pose.localTransforms[i] = BindTransformFromMatrix(skeleton.bones[i].localBindTransform);
    }
}

template <typename Key>
size_t FindKeyInterval(const std::vector<Key>& keys, float timeSeconds) {
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (timeSeconds <= keys[i + 1].timeSeconds) {
            return i;
        }
    }

    return keys.size() - 2;
}

glm::vec3 SampleVec3(const std::vector<TranslationKey>& keys, float timeSeconds, const glm::vec3& fallback) {
    if (keys.empty()) {
        return fallback;
    }
    if (keys.size() == 1 || timeSeconds <= keys.front().timeSeconds) {
        return keys.front().value;
    }
    if (timeSeconds >= keys.back().timeSeconds) {
        return keys.back().value;
    }

    const size_t i = FindKeyInterval(keys, timeSeconds);
    const TranslationKey& a = keys[i];
    const TranslationKey& b = keys[i + 1];
    const float duration = b.timeSeconds - a.timeSeconds;
    const float alpha = duration > 0.0f ? (timeSeconds - a.timeSeconds) / duration : 0.0f;
    return glm::mix(a.value, b.value, alpha);
}

glm::vec3 SampleVec3(const std::vector<ScaleKey>& keys, float timeSeconds, const glm::vec3& fallback) {
    if (keys.empty()) {
        return fallback;
    }
    if (keys.size() == 1 || timeSeconds <= keys.front().timeSeconds) {
        return keys.front().value;
    }
    if (timeSeconds >= keys.back().timeSeconds) {
        return keys.back().value;
    }

    const size_t i = FindKeyInterval(keys, timeSeconds);
    const ScaleKey& a = keys[i];
    const ScaleKey& b = keys[i + 1];
    const float duration = b.timeSeconds - a.timeSeconds;
    const float alpha = duration > 0.0f ? (timeSeconds - a.timeSeconds) / duration : 0.0f;
    return glm::mix(a.value, b.value, alpha);
}

glm::quat SampleQuat(const std::vector<RotationKey>& keys, float timeSeconds, const glm::quat& fallback) {
    if (keys.empty()) {
        return fallback;
    }
    if (keys.size() == 1 || timeSeconds <= keys.front().timeSeconds) {
        return glm::normalize(keys.front().value);
    }
    if (timeSeconds >= keys.back().timeSeconds) {
        return glm::normalize(keys.back().value);
    }

    const size_t i = FindKeyInterval(keys, timeSeconds);
    const RotationKey& a = keys[i];
    const RotationKey& b = keys[i + 1];
    const float duration = b.timeSeconds - a.timeSeconds;
    const float alpha = duration > 0.0f ? (timeSeconds - a.timeSeconds) / duration : 0.0f;
    return glm::normalize(glm::slerp(glm::normalize(a.value), glm::normalize(b.value), alpha));
}

} // namespace

void AnimationSampler::Sample(const AnimationClip& clip, const Skeleton& skeleton, float timeSeconds, AnimationPose& outPose) {
    SetBindPoseLocals(skeleton, outPose);
    outPose.modelSpaceMatrices.clear();
    outPose.skinningMatrices.clear();

    const float sampleTime = NormalizeSampleTime(clip, timeSeconds);
    for (const BoneTrack& track : clip.tracks) {
        if (track.boneIndex < 0 || static_cast<size_t>(track.boneIndex) >= outPose.localTransforms.size()) {
            continue;
        }

        LocalBoneTransform& local = outPose.localTransforms[static_cast<size_t>(track.boneIndex)];
        local.translation = SampleVec3(track.translations, sampleTime, local.translation);
        local.rotation = SampleQuat(track.rotations, sampleTime, local.rotation);
        local.scale = SampleVec3(track.scales, sampleTime, local.scale);
    }
}

void AnimationSampler::BuildModelSpacePose(const Skeleton& skeleton, AnimationPose& pose) {
    if (pose.localTransforms.size() != skeleton.bones.size()) {
        SetBindPoseLocals(skeleton, pose);
    }

    pose.modelSpaceMatrices.resize(skeleton.bones.size());
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        const glm::mat4 localMatrix = ComposeLocalMatrix(pose.localTransforms[i]);
        const int parent = skeleton.bones[i].parentIndex;
        if (parent >= 0 && static_cast<size_t>(parent) < i) {
            pose.modelSpaceMatrices[i] = pose.modelSpaceMatrices[static_cast<size_t>(parent)] * localMatrix;
        } else {
            pose.modelSpaceMatrices[i] = localMatrix;
        }
    }
}

void AnimationSampler::BuildSkinningMatrices(const Skeleton& skeleton, AnimationPose& pose) {
    if (pose.modelSpaceMatrices.size() != skeleton.bones.size()) {
        BuildModelSpacePose(skeleton, pose);
    }

    pose.skinningMatrices.resize(skeleton.bones.size());
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        pose.skinningMatrices[i] = pose.modelSpaceMatrices[i] * skeleton.bones[i].inverseBindPose;
    }
}

} // namespace Hockey
