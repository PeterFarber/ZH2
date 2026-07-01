#pragma once

#include "Hockey/Animation/AnimationClip.hpp"
#include "Hockey/Animation/AnimationPose.hpp"
#include "Hockey/Animation/Skeleton.hpp"

namespace Hockey {

class AnimationSampler {
public:
    static void Sample(const AnimationClip& clip, const Skeleton& skeleton, float timeSeconds, AnimationPose& outPose);
    static void BuildModelSpacePose(const Skeleton& skeleton, AnimationPose& pose);
    static void BuildSkinningMatrices(const Skeleton& skeleton, AnimationPose& pose);
};

} // namespace Hockey
