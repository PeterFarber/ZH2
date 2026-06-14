#include "Hockey/Physics/PhysicsWorld.hpp"

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Physics/Jolt/JoltPhysicsWorld.hpp"

namespace Hockey {

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

void PhysicsWorld::CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const {
    m_Impl->CollectDebugDraw(outDrawList);
}

} // namespace Hockey
