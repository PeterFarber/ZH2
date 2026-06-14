#include "Hockey/Physics/Jolt/JoltContactListener.hpp"

#include <utility>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>

#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

namespace Hockey::JoltDetail {

void JoltContactListener::RecordLocked(const JPH::Body& body) {
    m_BodyInfo[body.GetID()] = BodyInfo{UUID(body.GetUserData()), body.IsSensor()};
}

void JoltContactListener::EmitContact(const JPH::Body& body1, const JPH::Body& body2,
                                      const JPH::ContactManifold& manifold, PhysicsContactEvent::Type type) {
    PhysicsContactEvent event;
    event.entityA = UUID(body1.GetUserData());
    event.entityB = UUID(body2.GetUserData());
    event.contactNormal = ToGlm(manifold.mWorldSpaceNormal);
    if (manifold.mRelativeContactPointsOn1.size() > 0) {
        event.contactPoint = RToGlm(manifold.GetWorldSpaceContactPointOn1(0));
    }
    event.impulse = 0.0f; // Solver impulse is not available in this callback.
    event.type = type;
    m_Contacts.push_back(event);
}

void JoltContactListener::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
                                         const JPH::ContactManifold& manifold, JPH::ContactSettings&) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    RecordLocked(body1);
    RecordLocked(body2);

    if (body1.IsSensor() || body2.IsSensor()) {
        const JPH::Body& trigger = body1.IsSensor() ? body1 : body2;
        const JPH::Body& other = body1.IsSensor() ? body2 : body1;
        m_Triggers.push_back(PhysicsTriggerEvent{UUID(trigger.GetUserData()), UUID(other.GetUserData()),
                                                 PhysicsTriggerEvent::Type::Entered});
        return;
    }

    EmitContact(body1, body2, manifold, PhysicsContactEvent::Type::Started);
}

void JoltContactListener::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2,
                                             const JPH::ContactManifold& manifold, JPH::ContactSettings&) {
    if (body1.IsSensor() || body2.IsSensor()) {
        // Triggers only report enter/exit, not persistence.
        return;
    }
    std::lock_guard<std::mutex> lock(m_Mutex);
    EmitContact(body1, body2, manifold, PhysicsContactEvent::Type::Persisted);
}

void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    const auto it1 = m_BodyInfo.find(subShapePair.GetBody1ID());
    const auto it2 = m_BodyInfo.find(subShapePair.GetBody2ID());
    if (it1 == m_BodyInfo.end() || it2 == m_BodyInfo.end()) {
        // A body was destroyed/forgotten before its contact-removed fired; the
        // event simply can't be resolved. Skipping it is safe.
        return;
    }

    if (it1->second.isSensor || it2->second.isSensor) {
        const BodyInfo& trigger = it1->second.isSensor ? it1->second : it2->second;
        const BodyInfo& other = it1->second.isSensor ? it2->second : it1->second;
        m_Triggers.push_back(PhysicsTriggerEvent{trigger.uuid, other.uuid, PhysicsTriggerEvent::Type::Exited});
        return;
    }

    PhysicsContactEvent event;
    event.entityA = it1->second.uuid;
    event.entityB = it2->second.uuid;
    event.type = PhysicsContactEvent::Type::Ended;
    m_Contacts.push_back(event);
}

std::vector<PhysicsContactEvent> JoltContactListener::DrainContacts() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::vector<PhysicsContactEvent> out = std::move(m_Contacts);
    m_Contacts.clear();
    return out;
}

std::vector<PhysicsTriggerEvent> JoltContactListener::DrainTriggers() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::vector<PhysicsTriggerEvent> out = std::move(m_Triggers);
    m_Triggers.clear();
    return out;
}

void JoltContactListener::ForgetBody(const JPH::BodyID& bodyId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_BodyInfo.erase(bodyId);
}

void JoltContactListener::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Contacts.clear();
    m_Triggers.clear();
    m_BodyInfo.clear();
}

} // namespace Hockey::JoltDetail
