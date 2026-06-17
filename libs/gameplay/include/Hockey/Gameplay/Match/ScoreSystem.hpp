#pragma once

#include <cstdint>

#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class ScoreSystem {
public:
    static void AddGoal(Scene& scene, GameplayTeam scoringTeam, const GameplaySettings& settings, GameplayEventQueue& events);
    static uint32_t GetScore(Scene& scene, GameplayTeam team);
};

}
