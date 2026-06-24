#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

class Scene;

class ResetSystem {
public:
    static void BeginReset(Scene& scene, GameplayEventQueue& events, GameplayTeam causeTeam = GameplayTeam::None);
    static void CompleteReset(Scene& scene,
                              GameplayEventQueue& events,
                              GameplayTeam causeTeam = GameplayTeam::None,
                              const GameplaySettings& settings = {});
    static void FixedUpdate(Scene& scene, float fixedDeltaSeconds, const GameplaySettings& settings, GameplayEventQueue& events);
};

}
