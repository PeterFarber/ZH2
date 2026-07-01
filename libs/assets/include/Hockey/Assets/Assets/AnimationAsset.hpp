#pragma once

#include "Hockey/Assets/AssetID.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <vector>

namespace Hockey {

struct AnimationAssetTranslationKey {
    float timeSeconds = 0.0f;
    glm::vec3 value{0.0f};
};

struct AnimationAssetRotationKey {
    float timeSeconds = 0.0f;
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
};

struct AnimationAssetScaleKey {
    float timeSeconds = 0.0f;
    glm::vec3 value{1.0f};
};

struct AnimationBoneTrack {
    int boneIndex = -1;
    std::vector<AnimationAssetTranslationKey> translations;
    std::vector<AnimationAssetRotationKey> rotations;
    std::vector<AnimationAssetScaleKey> scales;
};

struct AnimationAssetEvent {
    float timeSeconds = 0.0f;
    std::string name;
};

struct AnimationAsset {
    AssetID id;
    std::string name;
    AssetID skeletonAsset;
    float durationSeconds = 0.0f;
    bool looping = true;
    std::vector<AnimationBoneTrack> tracks;
    std::vector<AnimationAssetEvent> events;
};

} // namespace Hockey
