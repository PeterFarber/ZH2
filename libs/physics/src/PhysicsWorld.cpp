#include "Hockey/Physics/PhysicsWorld.hpp"

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Jolt/JoltPhysicsWorld.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"

namespace Hockey {

namespace {

// Returns true if the trigger's detect flags accept a body on the given layer.
bool TriggerAcceptsLayer(const TriggerComponent& trigger, PhysicsLayer layer) {
    switch (layer) {
    case PhysicsLayer::Player:
        return trigger.detectPlayers;
    case PhysicsLayer::Goalie:
        return trigger.detectGoalies;
    case PhysicsLayer::Puck:
        return trigger.detectPuck;
    default:
        return true; // Other layers are not governed by the detect flags.
    }
}

bool TriggerAcceptsEvent(Scene& scene, const PhysicsTriggerEvent& event) {
    Entity trigger = scene.FindEntityByUUID(event.triggerEntity);
    if (!trigger.IsValid() || !trigger.HasComponent<TriggerComponent>()) {
        return true;
    }
    Entity other = scene.FindEntityByUUID(event.otherEntity);
    if (!other.IsValid() || !other.HasComponent<RigidBodyComponent>()) {
        return true;
    }
    return TriggerAcceptsLayer(trigger.GetComponent<TriggerComponent>(),
                               other.GetComponent<RigidBodyComponent>().layer);
}

} // namespace

PhysicsWorld::PhysicsWorld() : m_Impl(std::make_unique<JoltDetail::JoltPhysicsWorld>()) {}

PhysicsWorld::~PhysicsWorld() = default;

PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

Status PhysicsWorld::Init(const PhysicsSettings& settings) {
    return m_Impl->Init(settings);
}

void PhysicsWorld::Shutdown() {
    m_Impl->Shutdown();
}

bool PhysicsWorld::IsInitialized() const {
    return m_Impl->IsInitialized();
}

const PhysicsSettings& PhysicsWorld::Settings() const {
    return m_Impl->Settings();
}

void PhysicsWorld::SetGravity(const glm::vec3& gravity) {
    m_Impl->SetGravity(gravity);
}

glm::vec3 PhysicsWorld::GetGravity() const {
    return m_Impl->GetGravity();
}

void PhysicsWorld::CreateBodiesFromScene(Scene& scene) {
    m_Impl->CreateBodiesFromScene(scene);
}

void PhysicsWorld::DestroyBodies() {
    m_Impl->DestroyBodies();
}

void PhysicsWorld::Step(float fixedDeltaSeconds) {
    m_Impl->Step(fixedDeltaSeconds);
}

void PhysicsWorld::SyncSceneToPhysics(Scene& scene) {
    m_Impl->SyncSceneToPhysics(scene);
}

void PhysicsWorld::SyncPhysicsToScene(Scene& scene) {
    m_Impl->SyncPhysicsToScene(scene);
}

PhysicsBodyHandle PhysicsWorld::CreateBody(Entity entity) {
    return m_Impl->CreateBody(entity);
}

void PhysicsWorld::DestroyBody(Entity entity) {
    m_Impl->DestroyBody(entity);
}

bool PhysicsWorld::HasBody(Entity entity) const {
    return entity.IsValid() && m_Impl->HasBody(entity.GetUUID());
}

std::size_t PhysicsWorld::BodyCount() const {
    return m_Impl->BodyCount();
}

void PhysicsWorld::SetBodyTransform(Entity entity, const glm::vec3& position, const glm::quat& rotation) {
    if (entity.IsValid()) {
        m_Impl->SetBodyTransform(entity.GetUUID(), position, rotation);
    }
}

void PhysicsWorld::SetLinearVelocity(Entity entity, const glm::vec3& velocity) {
    if (entity.IsValid()) {
        m_Impl->SetLinearVelocity(entity.GetUUID(), velocity);
    }
}

void PhysicsWorld::AddForce(Entity entity, const glm::vec3& force) {
    if (entity.IsValid()) {
        m_Impl->AddForce(entity.GetUUID(), force);
    }
}

void PhysicsWorld::AddImpulse(Entity entity, const glm::vec3& impulse) {
    if (entity.IsValid()) {
        m_Impl->AddImpulse(entity.GetUUID(), impulse);
    }
}

glm::vec3 PhysicsWorld::GetLinearVelocity(Entity entity) const {
    return entity.IsValid() ? m_Impl->GetLinearVelocity(entity.GetUUID()) : glm::vec3(0.0f);
}

glm::vec3 PhysicsWorld::GetAngularVelocity(Entity entity) const {
    return entity.IsValid() ? m_Impl->GetAngularVelocity(entity.GetUUID()) : glm::vec3(0.0f);
}

bool PhysicsWorld::GetBodyPosition(Entity entity, glm::vec3& outPosition) const {
    return entity.IsValid() && m_Impl->GetBodyPosition(entity.GetUUID(), outPosition);
}

std::vector<PhysicsContactEvent> PhysicsWorld::DrainContactEvents() {
    return m_Impl->DrainContactEvents();
}

std::vector<PhysicsTriggerEvent> PhysicsWorld::DrainTriggerEvents() {
    return m_Impl->DrainTriggerEvents();
}

std::vector<PhysicsTriggerEvent> PhysicsWorld::DrainTriggerEvents(Scene& scene) {
    std::vector<PhysicsTriggerEvent> raw = m_Impl->DrainTriggerEvents();
    std::vector<PhysicsTriggerEvent> filtered;
    filtered.reserve(raw.size());
    for (const PhysicsTriggerEvent& event : raw) {
        if (TriggerAcceptsEvent(scene, event)) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

bool PhysicsWorld::Raycast(const RaycastRequest& request, RaycastHit& outHit) const {
    return m_Impl->Raycast(request, outHit);
}

std::vector<RaycastHit> PhysicsWorld::RaycastAll(const RaycastRequest& request) const {
    return m_Impl->RaycastAll(request);
}

bool PhysicsWorld::OverlapSphere(const OverlapSphereRequest& request, std::vector<OverlapHit>& outHits) const {
    return m_Impl->OverlapSphere(request, outHits);
}

bool PhysicsWorld::OverlapBox(const OverlapBoxRequest& request, std::vector<OverlapHit>& outHits) const {
    return m_Impl->OverlapBox(request, outHits);
}

bool PhysicsWorld::ShapeCast(const ShapeCastRequest& request, ShapeCastHit& outHit) const {
    return m_Impl->ShapeCast(request, outHit);
}

void PhysicsWorld::CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const {
    m_Impl->CollectDebugDraw(outDrawList);
}

} // namespace Hockey
