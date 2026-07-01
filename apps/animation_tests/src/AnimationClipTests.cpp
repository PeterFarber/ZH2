#include "Test.hpp"

#include "Hockey/Animation/AnimationClip.hpp"
#include "Hockey/Animation/Skeleton.hpp"

namespace {

Hockey::Skeleton MakeSkeleton() {
    Hockey::Skeleton skeleton;
    skeleton.bones.push_back({"Root", -1});
    skeleton.bones.push_back({"Child", 0});
    return skeleton;
}

Hockey::BoneTrack MakeSortedTrack() {
    Hockey::BoneTrack track;
    track.boneIndex = 1;
    track.translations.push_back({0.0f, {0.0f, 0.0f, 0.0f}});
    track.translations.push_back({1.0f, {1.0f, 0.0f, 0.0f}});
    track.rotations.push_back({0.0f, {1.0f, 0.0f, 0.0f, 0.0f}});
    track.rotations.push_back({1.0f, {1.0f, 0.0f, 0.0f, 0.0f}});
    track.scales.push_back({0.0f, {1.0f, 1.0f, 1.0f}});
    track.scales.push_back({1.0f, {2.0f, 2.0f, 2.0f}});
    return track;
}

} // namespace

void RunAnimationClipTests() {
    HockeyTest::BeginSuite("AnimationClip");

    const Hockey::Skeleton skeleton = MakeSkeleton();

    Hockey::AnimationClip clip;
    clip.name = "Skate";
    clip.durationSeconds = 1.0f;
    clip.looping = true;
    clip.tracks.push_back(MakeSortedTrack());
    clip.events.push_back({0.25f, "PlantLeftSkate"});
    clip.events.push_back({0.75f, "PlantRightSkate"});

    HK_CHECK(clip.looping);
    HK_CHECK_EQ(clip.durationSeconds, 1.0f);
    HK_CHECK(clip.HasTracks());
    HK_CHECK(clip.Validate(skeleton));

    Hockey::AnimationClip emptyClip;
    emptyClip.durationSeconds = 1.0f;
    HK_CHECK(!emptyClip.HasTracks());
    HK_CHECK(!emptyClip.Validate(skeleton));

    Hockey::AnimationClip badTrack = clip;
    badTrack.tracks[0].translations[1].timeSeconds = 0.1f;
    badTrack.tracks[0].translations[0].timeSeconds = 0.5f;
    HK_CHECK(!badTrack.Validate(skeleton));

    Hockey::AnimationClip badEvent = clip;
    badEvent.events[1].timeSeconds = 0.1f;
    HK_CHECK(!badEvent.Validate(skeleton));

    Hockey::AnimationClip badBone = clip;
    badBone.tracks[0].boneIndex = 5;
    HK_CHECK(!badBone.Validate(skeleton));
}
