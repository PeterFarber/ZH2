#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class PhysicsWorld;
class Scene;

class PlayerMovement {
public:
    static void FixedUpdate(Scene& scene,
                            PhysicsWorld* physicsWorld,
                            const GameplayInputBuffer& inputs,
                            const GameplayTuning& tuning,
                            float fixedDeltaSeconds,
                            GameplayEventQueue& events);
    static void SyncFromPhysics(Scene& scene, PhysicsWorld* physicsWorld);
};

}
