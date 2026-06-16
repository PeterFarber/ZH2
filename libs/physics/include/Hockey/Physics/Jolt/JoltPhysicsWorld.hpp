#pragma once

// Internal: this header pulls in Jolt and is only included from hockey_physics
// .cpp files (PhysicsWorld.cpp and the query/event/debug step files).
#include "Hockey/Physics/Jolt/JoltBroadPhaseLayer.hpp"
#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/Physics/PhysicsBody.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"
#include "Hockey/Physics/PhysicsQueries.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"

namespace Hockey {
class Scene;
class Entity;
} // namespace Hockey

namespace Hockey::JoltDetail {

class JoltContactListener;

// Lightweight description of a collider, cached per body so the debug pass can
// rebuild wireframes from the live Jolt transform without touching the scene.
struct DebugCollider {
    enum class Kind { Box, Sphere, Capsule, Cylinder };

    Kind kind = Kind::Box;
    glm::vec3 halfExtents{0.5f};
    float radius = 0.5f;
    float halfHeight = 0.5f;
    glm::vec3 offset{0.0f};
    glm::vec3 eulerDegrees{0.0f};
    bool isTrigger = false;
};

struct BodyDebugInfo {
    std::vector<DebugCollider> colliders;
};

// The real physics world. PhysicsWorld forwards to this; everything Jolt lives
// here so the public header stays Jolt-free.
class JoltPhysicsWorld {
public:
    JoltPhysicsWorld();
    ~JoltPhysicsWorld();

    JoltPhysicsWorld(const JoltPhysicsWorld&) = delete;
    JoltPhysicsWorld& operator=(const JoltPhysicsWorld&) = delete;

    Status Init(const PhysicsSettings& settings);
    void Shutdown();
    bool IsInitialized() const {
        return m_Initialized;
    }

    const PhysicsSettings& Settings() const {
        return m_Settings;
    }

    void SetGravity(const glm::vec3& gravity);
    glm::vec3 GetGravity() const;

    void CreateBodiesFromScene(Scene& scene);
    void DestroyBodies();

    void Step(float fixedDeltaSeconds);

    void SyncSceneToPhysics(Scene& scene);
    void SyncPhysicsToScene(Scene& scene);

    PhysicsBodyHandle CreateBody(Entity entity);
    void DestroyBody(Entity entity);
    bool HasBody(UUID entity) const;
    std::size_t BodyCount() const {
        return m_EntityToBody.size();
    }

    void SetBodyTransform(UUID entity, const glm::vec3& position, const glm::quat& rotation);
    void SetLinearVelocity(UUID entity, const glm::vec3& velocity);
    void AddForce(UUID entity, const glm::vec3& force);
    void AddImpulse(UUID entity, const glm::vec3& impulse);

    glm::vec3 GetLinearVelocity(UUID entity) const;
    glm::vec3 GetAngularVelocity(UUID entity) const;
    bool GetBodyPosition(UUID entity, glm::vec3& outPosition) const;

    std::vector<PhysicsContactEvent> DrainContactEvents();
    std::vector<PhysicsTriggerEvent> DrainTriggerEvents();

    bool Raycast(const RaycastRequest& request, RaycastHit& outHit) const;
    std::vector<RaycastHit> RaycastAll(const RaycastRequest& request) const;
    bool OverlapSphere(const OverlapSphereRequest& request, std::vector<OverlapHit>& outHits) const;
    bool OverlapBox(const OverlapBoxRequest& request, std::vector<OverlapHit>& outHits) const;
    bool ShapeCast(const ShapeCastRequest& request, ShapeCastHit& outHit) const;

    void CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const;

private:
    bool FindBody(UUID entity, JPH::BodyID& outId) const;
    bool ResolveEntity(const JPH::BodyID& bodyId, UUID& outEntity) const;
    PhysicsBodyHandle CreateBodyForEntity(const Entity& entity);
    void RemoveAndDestroy(JPH::BodyID id);

    bool m_Initialized = false;
    PhysicsSettings m_Settings;

    LayerInterfaces m_Layers;
    std::unique_ptr<JPH::PhysicsSystem> m_System;
    std::unique_ptr<JPH::TempAllocatorImpl> m_TempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;
    std::unique_ptr<JoltContactListener> m_ContactListener;

    std::unordered_map<UUID, JPH::BodyID> m_EntityToBody;
    std::unordered_map<JPH::BodyID, UUID, BodyIDHash> m_BodyToEntity;
    std::unordered_map<JPH::BodyID, BodyDebugInfo, BodyIDHash> m_BodyDebug;
};

} // namespace Hockey::JoltDetail
