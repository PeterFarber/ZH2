#pragma once

#include <glm/glm.hpp>

namespace Hockey {

class Entity;
struct StickTuning;

class StickHandling {
public:
    static glm::vec3 GetStickWorldPosition(Entity player);
    static glm::vec3 GetStickWorldPosition(Entity player, const StickTuning& tuning);
    static bool CanControlPuck(Entity player, Entity puck);
    static bool CanControlPuck(Entity player, Entity puck, const StickTuning& tuning);
};

}
