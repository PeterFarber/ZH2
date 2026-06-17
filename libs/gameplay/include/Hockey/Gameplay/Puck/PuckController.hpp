#pragma once

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class PuckController {
public:
    static void FixedUpdate(Scene& scene, const GameplayTuning& tuning, float fixedDeltaSeconds);
};

}
