#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class ResetSystem {
public:
    static void BeginReset(Scene& scene, GameplayEventQueue& events);
    static void CompleteReset(Scene& scene, GameplayEventQueue& events);
    static void FixedUpdate(Scene& scene, float fixedDeltaSeconds, const GameplaySettings& settings, GameplayEventQueue& events);
};

}
