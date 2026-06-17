#pragma once

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class GoalDetection {
public:
    static bool HandleGoalTrigger(Scene& scene,
                                  Entity goal,
                                  Entity other,
                                  const GameplaySettings& settings,
                                  GameplayEventQueue& events);
    static void FixedUpdate(Scene& scene, const GameplaySettings& settings, GameplayEventQueue& events);
};

}
