#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class FaceoffSystem {
public:
    static void FixedUpdate(Scene& scene,
                            float fixedDeltaSeconds,
                            const GameplaySettings& settings,
                            GameplayEventQueue& events);
};

}
