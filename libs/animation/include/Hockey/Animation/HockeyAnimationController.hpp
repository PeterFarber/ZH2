#pragma once

#include "Hockey/Core/UUID.hpp"

#include <glm/vec3.hpp>

#include <vector>

namespace Hockey {

class Scene;

enum class HockeyAnimationTriggerType {
    Shoot,
    Steal,
    GoalieSave,
    Celebrate
};

struct HockeyAnimationPlayerState {
    UUID entity{0};
    glm::vec3 velocity{0.0f};
    float shotChargeRatio = 0.0f;
    bool hasPuck = false;
    bool shotCharging = false;
    bool goalie = false;
};

struct HockeyAnimationTrigger {
    UUID entity{0};
    HockeyAnimationTriggerType type = HockeyAnimationTriggerType::Shoot;
};

struct HockeyAnimationFrame {
    std::vector<HockeyAnimationPlayerState> players;
    std::vector<HockeyAnimationTrigger> triggers;
};

class HockeyAnimationController {
public:
    void Apply(Scene& scene, const HockeyAnimationFrame& frame) const;
};

} // namespace Hockey
