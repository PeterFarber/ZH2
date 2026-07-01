#include "Test.hpp"

#include "Hockey/Animation/Animation.hpp"

void RunAnimationLibraryContractTests() {
    HockeyTest::BeginSuite("Animation library contract");

    HK_CHECK_EQ(Hockey::AnimationLibraryName(), std::string{"hockey_animation"});
}
