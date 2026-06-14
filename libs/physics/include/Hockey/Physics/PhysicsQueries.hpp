#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

struct RaycastRequest {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float maxDistance = 1000.0f;
    std::uint32_t layerMask = 0xFFFFFFFFu;
    bool includeTriggers = false;
};

struct RaycastHit {
    UUID entity;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};

struct OverlapSphereRequest {
    glm::vec3 center{0.0f};
    float radius = 1.0f;
    std::uint32_t layerMask = 0xFFFFFFFFu;
    bool includeTriggers = true;
};

struct OverlapBoxRequest {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    std::uint32_t layerMask = 0xFFFFFFFFu;
    bool includeTriggers = true;
};

struct OverlapHit {
    UUID entity;
};

} // namespace Hockey
