#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

class PhysicsWorld;
class Scene;

class ResetSystem {
public:
    static void BeginReset(Scene& scene,
                           GameplayEventQueue& events,
                           GameplayTeam causeTeam = GameplayTeam::None,
                           bool useNormalSpawnsForReset = false);
    static void CompleteReset(Scene& scene,
                              GameplayEventQueue& events,
                              GameplayTeam causeTeam = GameplayTeam::None,
                              bool useNormalSpawnsForReset = false,
                              const GameplaySettings& settings = {},
                              PhysicsWorld* physicsWorld = nullptr);
    static void FixedUpdate(Scene& scene,
                            float fixedDeltaSeconds,
                            const GameplaySettings& settings,
                            GameplayEventQueue& events,
                            PhysicsWorld* physicsWorld = nullptr);
};

}
