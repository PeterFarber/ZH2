#include "Hockey/Animation/AnimationClip.hpp"

#include "Hockey/Animation/Skeleton.hpp"

#include <limits>
#include <string>

namespace Hockey {
namespace {

template <typename Key>
Status ValidateKeyTimes(const std::vector<Key>& keys, float durationSeconds, const char* label) {
    float previousTime = -std::numeric_limits<float>::infinity();
    for (const Key& key : keys) {
        if (key.timeSeconds < 0.0f || key.timeSeconds > durationSeconds) {
            return Status::Fail(std::string(label) + " key is outside the clip duration");
        }
        if (key.timeSeconds < previousTime) {
            return Status::Fail(std::string(label) + " keys must be sorted by time");
        }
        previousTime = key.timeSeconds;
    }

    return Status::Ok();
}

} // namespace

bool AnimationClip::HasTracks() const {
    return !tracks.empty();
}

Status AnimationClip::Validate(const Skeleton& skeleton) const {
    if (durationSeconds <= 0.0f) {
        return Status::Fail("animation clip duration must be positive");
    }
    if (!HasTracks()) {
        return Status::Fail("animation clip must contain at least one track");
    }

    const Status skeletonStatus = skeleton.Validate(std::numeric_limits<uint32_t>::max());
    if (!skeletonStatus) {
        return skeletonStatus;
    }

    const int boneCount = static_cast<int>(skeleton.bones.size());
    for (const BoneTrack& track : tracks) {
        if (track.boneIndex < 0 || track.boneIndex >= boneCount) {
            return Status::Fail("animation track targets a missing bone");
        }
        if (track.translations.empty() && track.rotations.empty() && track.scales.empty()) {
            return Status::Fail("animation track must contain at least one keyframe");
        }

        if (const Status status = ValidateKeyTimes(track.translations, durationSeconds, "translation"); !status) {
            return status;
        }
        if (const Status status = ValidateKeyTimes(track.rotations, durationSeconds, "rotation"); !status) {
            return status;
        }
        if (const Status status = ValidateKeyTimes(track.scales, durationSeconds, "scale"); !status) {
            return status;
        }
    }

    float previousEventTime = -std::numeric_limits<float>::infinity();
    for (const AnimationEventKey& event : events) {
        if (event.timeSeconds < 0.0f || event.timeSeconds > durationSeconds) {
            return Status::Fail("animation event is outside the clip duration");
        }
        if (event.timeSeconds < previousEventTime) {
            return Status::Fail("animation events must be sorted by time");
        }
        previousEventTime = event.timeSeconds;
    }

    return Status::Ok();
}

} // namespace Hockey
