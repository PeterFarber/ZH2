#include "Test.hpp"

#include "Hockey/Animation/Skeleton.hpp"

namespace {

Hockey::Bone MakeBone(std::string name, int parentIndex) {
    Hockey::Bone bone;
    bone.name = std::move(name);
    bone.parentIndex = parentIndex;
    return bone;
}

} // namespace

void RunSkeletonTests() {
    HockeyTest::BeginSuite("Skeleton");

    Hockey::Skeleton parentChild;
    parentChild.name = "TwoBone";
    parentChild.bones.push_back(MakeBone("Root", -1));
    parentChild.bones.push_back(MakeBone("StickHand", 0));

    HK_CHECK(parentChild.Validate(128));
    HK_CHECK_EQ(parentChild.FindBone("Root"), 0);
    HK_CHECK_EQ(parentChild.FindBone("StickHand"), 1);
    HK_CHECK_EQ(parentChild.FindBone("Missing"), -1);

    Hockey::Skeleton invalidParent;
    invalidParent.bones.push_back(MakeBone("Root", -1));
    invalidParent.bones.push_back(MakeBone("BadChild", 7));
    HK_CHECK(!invalidParent.Validate(128));

    Hockey::Skeleton tooManyBones;
    tooManyBones.bones.push_back(MakeBone("Root", -1));
    tooManyBones.bones.push_back(MakeBone("Extra", 0));
    HK_CHECK(!tooManyBones.Validate(1));
}
