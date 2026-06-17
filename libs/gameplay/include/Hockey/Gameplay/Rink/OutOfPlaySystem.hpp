#pragma once

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class OutOfPlaySystem {
public:
    static bool IsPuckOutOfPlay(Scene& scene, Entity puck, const GameplaySettings& settings);
    static void HandleOutOfPlay(Scene& scene, const GameplaySettings& settings, GameplayEventQueue& events);
};

}
