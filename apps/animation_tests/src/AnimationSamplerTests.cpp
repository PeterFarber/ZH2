#include "Test.hpp"

#include "Hockey/Animation/AnimationSampler.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

Hockey::Skeleton MakeTwoBoneSkeleton() {
    Hockey::Skeleton skeleton;
    Hockey::Bone root;
    root.name = "Root";
    root.parentIndex = -1;
    root.localBindTransform = glm::translate(glm::mat4{1.0f}, {1.0f, 0.0f, 0.0f});
    skeleton.bones.push_back(root);

    Hockey::Bone child;
    child.name = "Child";
    child.parentIndex = 0;
    child.localBindTransform = glm::translate(glm::mat4{1.0f}, {0.0f, 2.0f, 0.0f});
    skeleton.bones.push_back(child);
    return skeleton;
}

Hockey::AnimationClip MakeTranslationClip(bool looping) {
    Hockey::AnimationClip clip;
    clip.durationSeconds = 1.0f;
    clip.looping = looping;

    Hockey::BoneTrack track;
    track.boneIndex = 0;
    track.translations.push_back({0.0f, {0.0f, 0.0f, 0.0f}});
    track.translations.push_back({1.0f, {10.0f, 0.0f, 0.0f}});
    clip.tracks.push_back(track);
    return clip;
}

} // namespace

void RunAnimationSamplerTests() {
    HockeyTest::BeginSuite("AnimationSampler");

    const Hockey::Skeleton skeleton = MakeTwoBoneSkeleton();

    Hockey::AnimationPose midpoint;
    Hockey::AnimationSampler::Sample(MakeTranslationClip(false), skeleton, 0.5f, midpoint);
    HK_CHECK_EQ(midpoint.localTransforms.size(), 2u);
    HK_CHECK_NEAR(midpoint.localTransforms[0].translation.x, 5.0f, 0.0001f);
    HK_CHECK_NEAR(midpoint.localTransforms[1].translation.y, 2.0f, 0.0001f);

    Hockey::AnimationClip rotationClip;
    rotationClip.durationSeconds = 1.0f;
    rotationClip.looping = false;
    Hockey::BoneTrack rotationTrack;
    rotationTrack.boneIndex = 0;
    rotationTrack.rotations.push_back({0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}});
    rotationTrack.rotations.push_back({1.0f, glm::angleAxis(glm::radians(90.0f), glm::vec3{0.0f, 0.0f, 1.0f})});
    rotationClip.tracks.push_back(rotationTrack);

    Hockey::AnimationPose rotationPose;
    Hockey::AnimationSampler::Sample(rotationClip, skeleton, 0.5f, rotationPose);
    const glm::vec3 rotated = rotationPose.localTransforms[0].rotation * glm::vec3{1.0f, 0.0f, 0.0f};
    HK_CHECK_NEAR(rotated.x, 0.7071f, 0.0001f);
    HK_CHECK_NEAR(rotated.y, 0.7071f, 0.0001f);

    Hockey::AnimationPose loopingPose;
    Hockey::AnimationSampler::Sample(MakeTranslationClip(true), skeleton, 1.5f, loopingPose);
    HK_CHECK_NEAR(loopingPose.localTransforms[0].translation.x, 5.0f, 0.0001f);

    Hockey::AnimationPose clampedPose;
    Hockey::AnimationSampler::Sample(MakeTranslationClip(false), skeleton, 2.0f, clampedPose);
    HK_CHECK_NEAR(clampedPose.localTransforms[0].translation.x, 10.0f, 0.0001f);

    Hockey::AnimationSampler::BuildModelSpacePose(skeleton, midpoint);
    HK_CHECK_EQ(midpoint.modelSpaceMatrices.size(), 2u);
    HK_CHECK_NEAR(midpoint.modelSpaceMatrices[1][3].x, 5.0f, 0.0001f);
    HK_CHECK_NEAR(midpoint.modelSpaceMatrices[1][3].y, 2.0f, 0.0001f);

    Hockey::Skeleton skinSkeleton;
    Hockey::Bone skinBone;
    skinBone.name = "SkinRoot";
    skinBone.inverseBindPose = glm::translate(glm::mat4{1.0f}, {-1.0f, 0.0f, 0.0f});
    skinSkeleton.bones.push_back(skinBone);

    Hockey::AnimationPose skinPose;
    skinPose.localTransforms.push_back({{3.0f, 0.0f, 0.0f}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
    Hockey::AnimationSampler::BuildModelSpacePose(skinSkeleton, skinPose);
    Hockey::AnimationSampler::BuildSkinningMatrices(skinSkeleton, skinPose);
    HK_CHECK_EQ(skinPose.skinningMatrices.size(), 1u);
    HK_CHECK_NEAR(skinPose.skinningMatrices[0][3].x, 2.0f, 0.0001f);
}
