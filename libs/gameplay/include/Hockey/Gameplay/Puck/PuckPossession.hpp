#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"

namespace Hockey {

class Entity;
class Scene;

class PuckPossession {
public:
    static bool TryAcquire(Scene& scene, Entity player, Entity puck, GameplayEventQueue& events);
    static void Release(Scene& scene, Entity puck, GameplayEventQueue& events);
    static void FixedUpdate(Scene& scene, GameplayEventQueue& events);
};

}
