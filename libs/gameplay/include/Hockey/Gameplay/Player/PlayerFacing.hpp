#pragma once

#include <glm/glm.hpp>

namespace Hockey {

class Entity;

void FacePlayerToward(Entity player, const glm::vec3& targetPosition);

} // namespace Hockey
