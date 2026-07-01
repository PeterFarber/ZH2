#include "Test.hpp"

#include "Hockey/Animation/AnimationBlend.hpp"

#include <glm/gtc/quaternion.hpp>

void RunAnimationBlendTests() {
    HockeyTest::BeginSuite("AnimationBlend");

    Hockey::AnimationPose a;
    a.localTransforms.push_back({{0.0f, 0.0f, 0.0f}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});

    Hockey::AnimationPose b;
    b.localTransforms.push_back(
        {{10.0f, 0.0f, 0.0f}, glm::angleAxis(glm::radians(90.0f), glm::vec3{0.0f, 0.0f, 1.0f}), {3.0f, 3.0f, 3.0f}});

    Hockey::AnimationPose blended;
    Hockey::AnimationBlend::BlendPoses(a, b, 0.5f, blended);

    HK_CHECK_EQ(blended.localTransforms.size(), 1u);
    HK_CHECK_NEAR(blended.localTransforms[0].translation.x, 5.0f, 0.0001f);
    HK_CHECK_NEAR(blended.localTransforms[0].scale.x, 2.0f, 0.0001f);

    const glm::vec3 rotated = blended.localTransforms[0].rotation * glm::vec3{1.0f, 0.0f, 0.0f};
    HK_CHECK_NEAR(rotated.x, 0.7071f, 0.0001f);
    HK_CHECK_NEAR(rotated.y, 0.7071f, 0.0001f);
}
