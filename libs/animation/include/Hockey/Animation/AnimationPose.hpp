#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace Hockey {

struct LocalBoneTransform {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

struct AnimationPose {
    std::vector<LocalBoneTransform> localTransforms;
    std::vector<glm::mat4> modelSpaceMatrices;
    std::vector<glm::mat4> skinningMatrices;
};

} // namespace Hockey
