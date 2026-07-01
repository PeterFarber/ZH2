#pragma once

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hockey {

struct Bone {
    std::string name;
    int parentIndex = -1;
    glm::mat4 inverseBindPose{1.0f};
    glm::mat4 localBindTransform{1.0f};
};

struct Skeleton {
    AssetID id;
    std::string name;
    std::vector<Bone> bones;

    int FindBone(std::string_view boneName) const;
    Status Validate(uint32_t maxBones) const;
};

} // namespace Hockey
