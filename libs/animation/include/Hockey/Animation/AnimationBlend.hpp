#pragma once

#include "Hockey/Animation/AnimationPose.hpp"

namespace Hockey {

class AnimationBlend {
public:
    static void BlendPoses(const AnimationPose& a, const AnimationPose& b, float alpha, AnimationPose& outPose);
};

} // namespace Hockey
