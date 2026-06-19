#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"

namespace Hockey {

class Scene;

class MatchClock {
public:
    static void FixedUpdate(Scene& scene, float fixedDeltaSeconds, GameplayEventQueue& events);
};

}
