#pragma once

#include "Hockey/Assets/AssetID.hpp"

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

namespace Hockey {

struct SkeletonAssetBone {
    std::string name;
    int parentIndex = -1;
    glm::mat4 inverseBindPose{1.0f};
    glm::mat4 localBindTransform{1.0f};
};

struct SkeletonAsset {
    AssetID id;
    std::string name;
    std::vector<SkeletonAssetBone> bones;
};

} // namespace Hockey
