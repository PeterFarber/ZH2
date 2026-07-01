#include "Test.hpp"

#include "Hockey/Animation/AnimationSettings.hpp"

void RunAnimationSettingsTests() {
    HockeyTest::BeginSuite("AnimationSettings");

    const Hockey::AnimationSettings settings;

    HK_CHECK(settings.enabled);
    HK_CHECK(settings.enableAnimationEvents);
    HK_CHECK(!settings.enableRootMotion);
    HK_CHECK_EQ(settings.defaultBlendTimeSeconds, 0.15f);
    HK_CHECK_EQ(settings.maxBones, 128u);
    HK_CHECK(!settings.debugDrawSkeletons);
    HK_CHECK(!settings.logAnimationEvents);
}
