#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"

namespace Hockey {

class Entity;
class PhysicsWorld;
class Scene;

class PuckPossession {
public:
    static bool TryAcquire(Scene& scene,
                           Entity player,
                           Entity puck,
                           GameplayEventQueue& events,
                           PhysicsWorld* physicsWorld = nullptr);
    static bool TryAcquire(Scene& scene,
                           Entity player,
                           Entity puck,
                           GameplayEventQueue& events,
                           PhysicsWorld* physicsWorld,
                           float puckFloorY);
    static void Release(Scene& scene, Entity puck, GameplayEventQueue& events);
    static void FixedUpdate(Scene& scene, GameplayEventQueue& events, PhysicsWorld* physicsWorld = nullptr);
    static void FixedUpdate(Scene& scene, GameplayEventQueue& events, PhysicsWorld* physicsWorld, float puckFloorY);
};

}
