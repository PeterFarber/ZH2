#pragma once

#include <glm/glm.hpp>

namespace Hockey {

class Entity;

class StickHandling {
public:
    static glm::vec3 GetStickWorldPosition(Entity player);
    static bool CanControlPuck(Entity player, Entity puck);
};

}
