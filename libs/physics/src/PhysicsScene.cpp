#include "Hockey/Physics/PhysicsScene.hpp"

#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

Status PhysicsScene::Init(Scene& scene, const PhysicsSettings& settings) {
    (void)scene;
    if (m_Initialized) {
        return Status::Ok();
    }

    Status status = m_World.Init(settings);
    if (!status) {
        return status;
    }

    m_Initialized = true;
    return Status::Ok();
}

void PhysicsScene::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    m_World.Shutdown();
    m_Initialized = false;
}

bool PhysicsScene::IsInitialized() const {
    return m_Initialized;
}

void PhysicsScene::OnSceneLoaded(Scene& scene) {
    if (!m_Initialized) {
        return;
    }
    m_World.DestroyBodies();
    m_World.CreateBodiesFromScene(scene);
}

void PhysicsScene::OnSceneUnloaded() {
    if (!m_Initialized) {
        return;
    }
    m_World.DestroyBodies();
}

void PhysicsScene::OnSimulationStart(Scene& scene) {
    if (!m_Initialized) {
        return;
    }
    m_World.DestroyBodies();
    m_World.CreateBodiesFromScene(scene);
}

void PhysicsScene::OnSimulationStop(Scene& scene) {
    (void)scene;
    if (!m_Initialized) {
        return;
    }
    // Clear any unconsumed events so a restart begins from a clean slate.
    m_World.DrainContactEvents();
    m_World.DrainTriggerEvents();
    m_World.DestroyBodies();
}

void PhysicsScene::OnFixedUpdate(Scene& scene, float fixedDeltaSeconds) {
    if (!m_Initialized || fixedDeltaSeconds <= 0.0f) {
        return;
    }

    // Push authored/kinematic transforms and react to entity add/remove first,
    // advance the simulation, then write dynamic results back into the scene.
    m_World.SyncSceneToPhysics(scene);
    m_World.Step(fixedDeltaSeconds);
    m_World.SyncPhysicsToScene(scene);
}

PhysicsWorld& PhysicsScene::World() {
    return m_World;
}

const PhysicsWorld& PhysicsScene::World() const {
    return m_World;
}

} // namespace Hockey
