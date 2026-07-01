#pragma once

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <vector>

namespace Hockey {

struct Skeleton;

struct TranslationKey {
    float timeSeconds = 0.0f;
    glm::vec3 value{0.0f};
};

struct RotationKey {
    float timeSeconds = 0.0f;
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
};

struct ScaleKey {
    float timeSeconds = 0.0f;
    glm::vec3 value{1.0f};
};

struct BoneTrack {
    int boneIndex = -1;
    std::vector<TranslationKey> translations;
    std::vector<RotationKey> rotations;
    std::vector<ScaleKey> scales;
};

struct AnimationEventKey {
    float timeSeconds = 0.0f;
    std::string name;
};

struct AnimationClip {
    AssetID id;
    std::string name;
    float durationSeconds = 0.0f;
    bool looping = true;
    std::vector<BoneTrack> tracks;
    std::vector<AnimationEventKey> events;

    bool HasTracks() const;
    Status Validate(const Skeleton& skeleton) const;
};

} // namespace Hockey
