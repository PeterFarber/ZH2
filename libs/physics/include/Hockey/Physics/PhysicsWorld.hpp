#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Physics/PhysicsBody.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"
#include "Hockey/Physics/PhysicsQueries.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"

namespace Hockey {

class Scene;
class Entity;

namespace JoltDetail {
class JoltPhysicsWorld;
}

// ---------------------------------------------------------------------------
// Owns a single Jolt physics simulation and the mapping between scene entities
// and Jolt bodies. All Jolt details are hidden behind a PIMPL so this header is
// renderer/Jolt-free and can be included anywhere (including the headless
// server). One PhysicsWorld drives one Scene.
// ---------------------------------------------------------------------------
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&&) noexcept;

    Status Init(const PhysicsSettings& settings);
    void Shutdown();
    bool IsInitialized() const;

    const PhysicsSettings& Settings() const;

    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

    // --- scene <-> physics --------------------------------------------------
    void CreateBodiesFromScene(Scene& scene);
    void DestroyBodies();

    void Step(float fixedDeltaSeconds);

    void SyncSceneToPhysics(Scene& scene);
    void SyncPhysicsToScene(Scene& scene);

    // --- per-body control ---------------------------------------------------
    PhysicsBodyHandle CreateBody(Entity entity);
    void DestroyBody(Entity entity);
    void SuspendBody(Entity entity);
    PhysicsBodyHandle ResumeBody(Entity entity);
    bool HasBody(Entity entity) const;

    std::size_t BodyCount() const;

    void SetBodyTransform(Entity entity, const glm::vec3& position, const glm::quat& rotation);
    void SetLinearVelocity(Entity entity, const glm::vec3& velocity);
    void AddForce(Entity entity, const glm::vec3& force);
    void AddImpulse(Entity entity, const glm::vec3& impulse);

    glm::vec3 GetLinearVelocity(Entity entity) const;
    glm::vec3 GetAngularVelocity(Entity entity) const;

    bool GetBodyPosition(Entity entity, glm::vec3& outPosition) const;

    // --- events (Step 7) ----------------------------------------------------
    std::vector<PhysicsContactEvent> DrainContactEvents();
    std::vector<PhysicsTriggerEvent> DrainTriggerEvents();

    // Drains trigger events and drops those rejected by the trigger's
    // TriggerComponent detect flags (detectPlayers/detectGoalies/detectPuck),
    // matched against the other body's RigidBodyComponent layer. Events whose
    // trigger entity has no TriggerComponent are passed through unchanged.
    std::vector<PhysicsTriggerEvent> DrainTriggerEvents(Scene& scene);

    // --- queries (Step 8) ---------------------------------------------------
    bool Raycast(const RaycastRequest& request, RaycastHit& outHit) const;
    std::vector<RaycastHit> RaycastAll(const RaycastRequest& request) const;
    bool OverlapSphere(const OverlapSphereRequest& request, std::vector<OverlapHit>& outHits) const;
    bool OverlapBox(const OverlapBoxRequest& request, std::vector<OverlapHit>& outHits) const;
    bool ShapeCast(const ShapeCastRequest& request, ShapeCastHit& outHit) const;

    // --- debug draw (Step 9) ------------------------------------------------
    void CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const;

private:
    std::unique_ptr<JoltDetail::JoltPhysicsWorld> m_Impl;
};

} // namespace Hockey
