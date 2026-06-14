#pragma once

#include "Hockey/Core/Result.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {

class Scene;

// ---------------------------------------------------------------------------
// Owns a PhysicsWorld for one ECS Scene and drives its lifecycle. Free of any
// renderer/Jolt dependency so it can be used headless on the dedicated server.
//
// Typical flow:
//   Init                 -> create the simulation
//   OnSceneLoaded        -> (re)build bodies for the current scene contents
//   OnSimulationStart    -> bodies ready, simulation about to run
//   OnFixedUpdate (xN)   -> sync scene -> step -> sync back
//   OnSimulationStop     -> tear down runtime bodies, drain pending events
//   OnSceneUnloaded      -> destroy bodies
//   Shutdown             -> destroy the simulation
// ---------------------------------------------------------------------------
class PhysicsScene {
public:
    PhysicsScene() = default;
    ~PhysicsScene() = default;

    PhysicsScene(const PhysicsScene&) = delete;
    PhysicsScene& operator=(const PhysicsScene&) = delete;

    Status Init(Scene& scene, const PhysicsSettings& settings);
    void Shutdown();

    bool IsInitialized() const;

    void OnSceneLoaded(Scene& scene);
    void OnSceneUnloaded();

    void OnSimulationStart(Scene& scene);
    void OnSimulationStop(Scene& scene);

    void OnFixedUpdate(Scene& scene, float fixedDeltaSeconds);

    PhysicsWorld& World();
    const PhysicsWorld& World() const;

private:
    PhysicsWorld m_World;
    bool m_Initialized = false;
};

} // namespace Hockey
