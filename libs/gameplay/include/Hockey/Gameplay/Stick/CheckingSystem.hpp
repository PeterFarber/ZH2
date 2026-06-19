#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class Scene;

class CheckingSystem {
public:
    static void FixedUpdate(Scene& scene,
                            const GameplayInputBuffer& inputs,
                            const GameplaySettings& settings,
                            const GameplayTuning& tuning,
                            float fixedDeltaSeconds,
                            GameplayEventQueue& events);
};

}
