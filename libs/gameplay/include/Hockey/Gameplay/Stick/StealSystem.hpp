#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class PhysicsWorld;
class Scene;

class StealSystem {
public:
    static void FixedUpdate(Scene& scene,
                            const GameplayInputBuffer& inputs,
                            const GameplayTuning& tuning,
                            float fixedDeltaSeconds,
                            GameplayEventQueue& events,
                            PhysicsWorld* physicsWorld = nullptr);
};

}
