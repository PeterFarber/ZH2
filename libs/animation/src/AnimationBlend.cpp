#include "Hockey/Animation/AnimationBlend.hpp"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace Hockey {

void AnimationBlend::BlendPoses(const AnimationPose& a, const AnimationPose& b, float alpha, AnimationPose& outPose) {
    const float t = std::clamp(alpha, 0.0f, 1.0f);
    const size_t count = std::max(a.localTransforms.size(), b.localTransforms.size());

    outPose.localTransforms.resize(count);
    outPose.modelSpaceMatrices.clear();
    outPose.skinningMatrices.clear();

    for (size_t i = 0; i < count; ++i) {
        const LocalBoneTransform left = i < a.localTransforms.size() ? a.localTransforms[i] : LocalBoneTransform{};
        const LocalBoneTransform right = i < b.localTransforms.size() ? b.localTransforms[i] : LocalBoneTransform{};

        LocalBoneTransform blended;
        blended.translation = glm::mix(left.translation, right.translation, t);
        blended.rotation = glm::normalize(glm::slerp(glm::normalize(left.rotation), glm::normalize(right.rotation), t));
        blended.scale = glm::mix(left.scale, right.scale, t);
        outPose.localTransforms[i] = blended;
    }
}

} // namespace Hockey
