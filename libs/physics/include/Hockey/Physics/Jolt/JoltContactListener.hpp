#pragma once

// Internal: pulls in Jolt. Only included from hockey_physics .cpp files.
#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

#include <Jolt/Physics/Collision/ContactListener.h>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

namespace Hockey::JoltDetail {

// Receives Jolt contact callbacks (on worker threads during Update) and turns
// them into engine-level contact/trigger events keyed by entity UUID. Bodies
// store their owning entity's UUID in mUserData, so most events resolve without
// touching the scene. Events are drained once per tick by the world.
//
// Trigger (sensor) bodies are reported as PhysicsTriggerEvent; everything else
// as PhysicsContactEvent.
class JoltContactListener final : public JPH::ContactListener {
public:
    void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold,
                        JPH::ContactSettings& settings) override;
    void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold,
                            JPH::ContactSettings& settings) override;
    void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;

    std::vector<PhysicsContactEvent> DrainContacts();
    std::vector<PhysicsTriggerEvent> DrainTriggers();

    // Associates a body with the surface material it was built from so the
    // contact callbacks can apply the material combine modes. Called by the
    // world right after the body is created.
    void RegisterBodyMaterial(const JPH::BodyID& bodyId, const PhysicsMaterial& material);

    // Drops cached info for a destroyed body so the cache stays bounded.
    void ForgetBody(const JPH::BodyID& bodyId);
    void Clear();

private:
    struct BodyInfo {
        UUID uuid;
        bool isSensor = false;
    };

    // Records body identity/sensor flag so OnContactRemoved (which only gets
    // body IDs, never Body&) can still resolve the entities. Caller must hold
    // m_Mutex.
    void RecordLocked(const JPH::Body& body);

    void EmitContact(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold,
                     PhysicsContactEvent::Type type);

    // Applies the per-body material combine modes to a contact, if both bodies'
    // materials are known. Caller must hold m_Mutex.
    void ApplyCombineModesLocked(const JPH::Body& body1, const JPH::Body& body2, JPH::ContactSettings& settings) const;

    std::mutex m_Mutex;
    std::vector<PhysicsContactEvent> m_Contacts;
    std::vector<PhysicsTriggerEvent> m_Triggers;
    std::unordered_map<JPH::BodyID, BodyInfo, BodyIDHash> m_BodyInfo;
    std::unordered_map<JPH::BodyID, PhysicsMaterial, BodyIDHash> m_BodyMaterials;
};

} // namespace Hockey::JoltDetail
