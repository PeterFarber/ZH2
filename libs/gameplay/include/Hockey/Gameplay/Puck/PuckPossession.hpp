#pragma once

#include "Hockey/Gameplay/GameplayEvents.hpp"

namespace Hockey {

class Entity;
class PhysicsWorld;
class Scene;
struct GameplayTuning;

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
    static bool TryAcquire(Scene& scene,
                           Entity player,
                           Entity puck,
                           GameplayEventQueue& events,
                           PhysicsWorld* physicsWorld,
                           const GameplayTuning& tuning);
    static void Release(Scene& scene, Entity puck, GameplayEventQueue& events, PhysicsWorld* physicsWorld = nullptr);
    static void FixedUpdate(Scene& scene, GameplayEventQueue& events, PhysicsWorld* physicsWorld = nullptr);
    static void FixedUpdate(Scene& scene, GameplayEventQueue& events, PhysicsWorld* physicsWorld, float puckFloorY);
    static void FixedUpdate(Scene& scene,
                            GameplayEventQueue& events,
                            PhysicsWorld* physicsWorld,
                            const GameplayTuning& tuning);
};

}
