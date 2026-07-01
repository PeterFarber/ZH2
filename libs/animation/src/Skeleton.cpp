#include "Hockey/Animation/Skeleton.hpp"

#include <string>

namespace Hockey {

int Skeleton::FindBone(std::string_view boneName) const {
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].name == boneName) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

Status Skeleton::Validate(uint32_t maxBones) const {
    if (bones.size() > maxBones) {
        return Status::Fail("skeleton has more bones than the configured maximum");
    }

    const int boneCount = static_cast<int>(bones.size());
    for (int i = 0; i < boneCount; ++i) {
        const int parent = bones[static_cast<size_t>(i)].parentIndex;
        if (parent < -1 || parent >= boneCount) {
            return Status::Fail("skeleton bone parent index is outside the bone array");
        }
        if (parent == i) {
            return Status::Fail("skeleton bone cannot parent itself");
        }
    }

    return Status::Ok();
}

} // namespace Hockey
