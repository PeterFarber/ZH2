#include "Hockey/Physics/Jolt/JoltPhysicsWorld.hpp"

#include <thread>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/PhysicsSettings.h>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Jolt/JoltBodyInterface.hpp"
#include "Hockey/Physics/Jolt/JoltContactListener.hpp"
#include "Hockey/Physics/Jolt/JoltShapeFactory.hpp"
#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

namespace Hockey::JoltDetail {

namespace {

PhysicsBodyHandle MakeHandle(JPH::BodyID id) {
    if (id.IsInvalid()) {
        return {};
    }
    // +1 keeps 0 reserved as the "invalid handle" sentinel.
    return PhysicsBodyHandle{id.GetIndexAndSequenceNumber() + 1u};
}

void Decompose(const glm::mat4& m, glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale) {
    outPos = glm::vec3(m[3]);

    const glm::vec3 c0(m[0]);
    const glm::vec3 c1(m[1]);
    const glm::vec3 c2(m[2]);
    outScale = glm::vec3(glm::length(c0), glm::length(c1), glm::length(c2));

    const glm::vec3 r0 = outScale.x > 1e-8f ? c0 / outScale.x : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 r1 = outScale.y > 1e-8f ? c1 / outScale.y : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 r2 = outScale.z > 1e-8f ? c2 / outScale.z : glm::vec3(0.0f, 0.0f, 1.0f);
    outRot = glm::quat_cast(glm::mat3(r0, r1, r2));
}

glm::mat4 Compose(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale) {
    return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);
}

bool QualifiesForBody(const Entity& entity) {
    return entity.HasComponent<RigidBodyComponent>() && HasAnyCollider(entity);
}

// Captures a compact description of every collider on the entity for the debug
// draw pass (which rebuilds wireframes from the live Jolt transform).
BodyDebugInfo CaptureDebugColliders(const Entity& entity) {
    BodyDebugInfo info;
    if (entity.HasComponent<BoxColliderComponent>()) {
        const auto& c = entity.GetComponent<BoxColliderComponent>();
        DebugCollider d;
        d.kind = DebugCollider::Kind::Box;
        d.halfExtents = c.halfExtents;
        d.offset = c.offset;
        d.eulerDegrees = c.rotation;
        d.isTrigger = c.isTrigger;
        info.colliders.push_back(d);
    }
    if (entity.HasComponent<SphereColliderComponent>()) {
        const auto& c = entity.GetComponent<SphereColliderComponent>();
        DebugCollider d;
        d.kind = DebugCollider::Kind::Sphere;
        d.radius = c.radius;
        d.offset = c.offset;
        d.isTrigger = c.isTrigger;
        info.colliders.push_back(d);
    }
    if (entity.HasComponent<CapsuleColliderComponent>()) {
        const auto& c = entity.GetComponent<CapsuleColliderComponent>();
        DebugCollider d;
        d.kind = DebugCollider::Kind::Capsule;
        d.radius = c.radius;
        d.halfHeight = c.halfHeight;
        d.offset = c.offset;
        d.eulerDegrees = c.rotation;
        d.isTrigger = c.isTrigger;
        info.colliders.push_back(d);
    }
    if (entity.HasComponent<CylinderColliderComponent>()) {
        const auto& c = entity.GetComponent<CylinderColliderComponent>();
        DebugCollider d;
        d.kind = DebugCollider::Kind::Cylinder;
        d.radius = c.radius;
        d.halfHeight = c.halfHeight;
        d.offset = c.offset;
        d.eulerDegrees = c.rotation;
        d.isTrigger = c.isTrigger;
        info.colliders.push_back(d);
    }
    return info;
}

int ResolveThreadCount(const PhysicsSettings& settings) {
    if (settings.jobThreadCount > 0) {
        return static_cast<int>(settings.jobThreadCount);
    }
    const unsigned hc = std::thread::hardware_concurrency();
    return hc > 1u ? static_cast<int>(hc) - 1 : 1;
}

} // namespace

JoltPhysicsWorld::JoltPhysicsWorld() = default;

JoltPhysicsWorld::~JoltPhysicsWorld() {
    Shutdown();
}

Status JoltPhysicsWorld::Init(const PhysicsSettings& settings) {
    if (m_Initialized) {
        Shutdown();
    }

    m_Settings = settings;

    const std::size_t tempBytes = static_cast<std::size_t>(settings.tempAllocatorSizeMB) * 1024u * 1024u;
    m_TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(tempBytes);
    m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                                             ResolveThreadCount(settings));

    m_System = std::make_unique<JPH::PhysicsSystem>();
    m_System->Init(settings.maxBodies, /*numBodyMutexes*/ 0, settings.maxBodyPairs, settings.maxContactConstraints,
                   m_Layers.broadPhase, m_Layers.objectVsBroadPhase, m_Layers.objectPair);
    m_System->SetGravity(ToJolt(settings.gravity));

    // Apply tunables that map onto Jolt's own PhysicsSettings. deterministicMode
    // controls reproducibility (required for the authoritative server);
    // integrationSubsteps is applied per-Step (see Step()).
    {
        JPH::PhysicsSettings joltSettings = m_System->GetPhysicsSettings();
        joltSettings.mDeterministicSimulation = settings.deterministicMode;
        m_System->SetPhysicsSettings(joltSettings);
    }

    m_ContactListener = std::make_unique<JoltContactListener>();
    m_System->SetContactListener(m_ContactListener.get());

    m_Initialized = true;
    HK_CORE_INFO("PhysicsWorld initialised (maxBodies={}, gravity=({:.2f},{:.2f},{:.2f}))", settings.maxBodies,
                 settings.gravity.x, settings.gravity.y, settings.gravity.z);
    return Status::Ok();
}

void JoltPhysicsWorld::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    DestroyBodies();
    m_SuspendedBodies.clear();
    m_System.reset();
    m_ContactListener.reset();
    m_JobSystem.reset();
    m_TempAllocator.reset();
    m_Initialized = false;
}

void JoltPhysicsWorld::SetGravity(const glm::vec3& gravity) {
    m_Settings.gravity = gravity;
    if (m_System) {
        m_System->SetGravity(ToJolt(gravity));
    }
}

glm::vec3 JoltPhysicsWorld::GetGravity() const {
    if (m_System) {
        return ToGlm(m_System->GetGravity());
    }
    return m_Settings.gravity;
}

bool JoltPhysicsWorld::FindBody(UUID entity, JPH::BodyID& outId) const {
    const auto it = m_EntityToBody.find(entity);
    if (it == m_EntityToBody.end()) {
        return false;
    }
    outId = it->second;
    return true;
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBodyForEntity(const Entity& entity) {
    if (!m_Initialized || !entity.IsValid()) {
        return {};
    }
    if (!QualifiesForBody(entity)) {
        return {};
    }

    const UUID uuid = entity.GetUUID();
    if (const auto it = m_EntityToBody.find(uuid); it != m_EntityToBody.end()) {
        return MakeHandle(it->second);
    }

    std::string error;
    bool allTriggers = false;
    bool anyTrigger = false;
    JPH::Ref<JPH::Shape> shape = CreateEntityShape(entity, error, allTriggers, anyTrigger);
    if (shape == nullptr) {
        HK_CORE_WARN("Physics body skipped for entity '{}': {}", entity.GetName(), error);
        return {};
    }

    const auto& rb = entity.GetComponent<RigidBodyComponent>();
    // A PhysicsMaterialComponent, when present, overrides the rigid body's
    // material name (lets entities share a rigid-body setup but tune surface).
    const std::string& materialName = entity.template HasComponent<PhysicsMaterialComponent>()
                                          ? entity.template GetComponent<PhysicsMaterialComponent>().materialName
                                          : rb.materialName;
    const PhysicsMaterial& material = PhysicsMaterialRegistry::Get().FindOrDefault(materialName);

    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale;
    Decompose(entity.GetScene()->GetWorldTransform(entity), pos, rot, scale);

    const bool isSensor = allTriggers;
    JPH::BodyCreationSettings creation = MakeBodyCreationSettings(rb, material, shape, ToJoltR(pos), ToJolt(rot),
                                                                  isSensor, m_Settings.enableSleeping, uuid.Value());

    JPH::BodyInterface& bodyInterface = m_System->GetBodyInterface();
    const JPH::EActivation activation =
        rb.type == RigidBodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
    const JPH::BodyID id = bodyInterface.CreateAndAddBody(creation, activation);
    if (id.IsInvalid()) {
        HK_CORE_ERROR("Physics body creation failed for entity '{}' (body limit reached?)", entity.GetName());
        return {};
    }

    m_EntityToBody.emplace(uuid, id);
    m_BodyToEntity.emplace(id, uuid);
    m_BodyDebug.emplace(id, CaptureDebugColliders(entity));
    if (m_ContactListener) {
        // Let the contact callbacks resolve the surface combine modes.
        m_ContactListener->RegisterBodyMaterial(id, material);
    }
    return MakeHandle(id);
}

void JoltPhysicsWorld::RemoveAndDestroy(JPH::BodyID id) {
    if (id.IsInvalid() || !m_System) {
        return;
    }
    JPH::BodyInterface& bodyInterface = m_System->GetBodyInterface();
    if (bodyInterface.IsAdded(id)) {
        bodyInterface.RemoveBody(id);
    }
    bodyInterface.DestroyBody(id);
    if (m_ContactListener) {
        m_ContactListener->ForgetBody(id);
    }
}

void JoltPhysicsWorld::CreateBodiesFromScene(Scene& scene) {
    if (!m_Initialized) {
        return;
    }
    for (Entity entity : scene.GetAllEntities()) {
        if (QualifiesForBody(entity) && m_SuspendedBodies.find(entity.GetUUID()) == m_SuspendedBodies.end()) {
            CreateBodyForEntity(entity);
        }
    }
    m_System->OptimizeBroadPhase();
}

void JoltPhysicsWorld::DestroyBodies() {
    if (!m_System) {
        m_EntityToBody.clear();
        m_BodyToEntity.clear();
        m_SuspendedBodies.clear();
        return;
    }
    for (const auto& [uuid, id] : m_EntityToBody) {
        (void)uuid;
        RemoveAndDestroy(id);
    }
    m_EntityToBody.clear();
    m_BodyToEntity.clear();
    m_BodyDebug.clear();
    m_SuspendedBodies.clear();
}

void JoltPhysicsWorld::Step(float fixedDeltaSeconds) {
    if (!m_Initialized || fixedDeltaSeconds <= 0.0f) {
        return;
    }
    const int collisionSteps = static_cast<int>(m_Settings.collisionSteps > 0 ? m_Settings.collisionSteps : 1);
    // integrationSubsteps divides the fixed step into N equal Jolt updates. A
    // value of 1 (default) reproduces the original single-update behaviour.
    const int substeps = static_cast<int>(m_Settings.integrationSubsteps > 0 ? m_Settings.integrationSubsteps : 1);
    const float subDelta = fixedDeltaSeconds / static_cast<float>(substeps);
    for (int i = 0; i < substeps; ++i) {
        m_System->Update(subDelta, collisionSteps, m_TempAllocator.get(), m_JobSystem.get());
    }
}

PhysicsBodyHandle JoltPhysicsWorld::CreateBody(Entity entity) {
    if (entity.IsValid()) {
        m_SuspendedBodies.erase(entity.GetUUID());
    }
    return CreateBodyForEntity(entity);
}

void JoltPhysicsWorld::DestroyBody(Entity entity) {
    if (!entity.IsValid()) {
        return;
    }
    const UUID uuid = entity.GetUUID();
    m_SuspendedBodies.erase(uuid);
    const auto it = m_EntityToBody.find(uuid);
    if (it == m_EntityToBody.end()) {
        return;
    }
    const JPH::BodyID id = it->second;
    RemoveAndDestroy(id);
    m_EntityToBody.erase(it);
    m_BodyToEntity.erase(id);
    m_BodyDebug.erase(id);
}

void JoltPhysicsWorld::SuspendBody(Entity entity) {
    if (!entity.IsValid()) {
        return;
    }

    const UUID uuid = entity.GetUUID();
    m_SuspendedBodies.insert(uuid);

    const auto it = m_EntityToBody.find(uuid);
    if (it == m_EntityToBody.end()) {
        return;
    }

    const JPH::BodyID id = it->second;
    RemoveAndDestroy(id);
    m_EntityToBody.erase(it);
    m_BodyToEntity.erase(id);
    m_BodyDebug.erase(id);
}

PhysicsBodyHandle JoltPhysicsWorld::ResumeBody(Entity entity) {
    if (!entity.IsValid()) {
        return {};
    }

    m_SuspendedBodies.erase(entity.GetUUID());
    return CreateBodyForEntity(entity);
}

bool JoltPhysicsWorld::HasBody(UUID entity) const {
    return m_EntityToBody.find(entity) != m_EntityToBody.end();
}

void JoltPhysicsWorld::SyncSceneToPhysics(Scene& scene) {
    if (!m_Initialized) {
        return;
    }

    JPH::BodyInterface& bodyInterface = m_System->GetBodyInterface();

    std::vector<Entity> entities = scene.GetAllEntities();
    std::unordered_map<UUID, bool> qualifying;
    qualifying.reserve(entities.size());

    for (Entity entity : entities) {
        if (!QualifiesForBody(entity)) {
            continue;
        }
        const UUID uuid = entity.GetUUID();
        if (m_SuspendedBodies.find(uuid) != m_SuspendedBodies.end()) {
            const auto suspendedBody = m_EntityToBody.find(uuid);
            if (suspendedBody != m_EntityToBody.end()) {
                const JPH::BodyID id = suspendedBody->second;
                RemoveAndDestroy(id);
                m_EntityToBody.erase(suspendedBody);
                m_BodyToEntity.erase(id);
                m_BodyDebug.erase(id);
            }
            continue;
        }

        qualifying.emplace(uuid, true);

        const auto bodyIt = m_EntityToBody.find(uuid);
        if (bodyIt == m_EntityToBody.end()) {
            CreateBodyForEntity(entity);
            continue;
        }

        // Drive kinematic and static bodies from the scene transform. Dynamic
        // bodies are owned by the simulation and never overwritten here.
        const auto& rb = entity.GetComponent<RigidBodyComponent>();
        if (rb.type == RigidBodyType::Dynamic) {
            continue;
        }

        glm::vec3 pos;
        glm::quat rot;
        glm::vec3 scale;
        Decompose(scene.GetWorldTransform(entity), pos, rot, scale);
        const JPH::EActivation activation =
            rb.type == RigidBodyType::Kinematic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
        bodyInterface.SetPositionAndRotation(bodyIt->second, ToJoltR(pos), ToJolt(rot), activation);
    }

    // Destroy bodies whose entity no longer qualifies (removed entity or
    // removed rigid body / collider components).
    std::vector<UUID> toRemove;
    for (const auto& [uuid, id] : m_EntityToBody) {
        if (qualifying.find(uuid) == qualifying.end()) {
            toRemove.push_back(uuid);
        }
    }
    for (UUID uuid : toRemove) {
        const auto it = m_EntityToBody.find(uuid);
        if (it != m_EntityToBody.end()) {
            const JPH::BodyID id = it->second;
            RemoveAndDestroy(id);
            m_EntityToBody.erase(it);
            m_BodyToEntity.erase(id);
            m_BodyDebug.erase(id);
        }
    }
}

void JoltPhysicsWorld::SyncPhysicsToScene(Scene& scene) {
    if (!m_Initialized) {
        return;
    }

    JPH::BodyInterface& bodyInterface = m_System->GetBodyInterface();

    for (const auto& [uuid, id] : m_EntityToBody) {
        Entity entity = scene.FindEntityByUUID(uuid);
        if (!entity.IsValid() || !entity.HasComponent<RigidBodyComponent>()) {
            continue;
        }
        if (entity.GetComponent<RigidBodyComponent>().type != RigidBodyType::Dynamic) {
            continue;
        }

        JPH::RVec3 jpos;
        JPH::Quat jrot;
        bodyInterface.GetPositionAndRotation(id, jpos, jrot);

        glm::vec3 oldPos;
        glm::quat oldRot;
        glm::vec3 scale;
        Decompose(scene.GetWorldTransform(entity), oldPos, oldRot, scale);

        scene.SetWorldTransform(entity, Compose(RToGlm(jpos), ToGlm(jrot), scale));
    }
}

void JoltPhysicsWorld::SetBodyTransform(UUID entity, const glm::vec3& position, const glm::quat& rotation) {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return;
    }
    m_System->GetBodyInterface().SetPositionAndRotation(id, ToJoltR(position), ToJolt(rotation),
                                                        JPH::EActivation::Activate);
}

void JoltPhysicsWorld::SetLinearVelocity(UUID entity, const glm::vec3& velocity) {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return;
    }
    m_System->GetBodyInterface().SetLinearVelocity(id, ToJolt(velocity));
}

void JoltPhysicsWorld::AddForce(UUID entity, const glm::vec3& force) {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return;
    }
    m_System->GetBodyInterface().AddForce(id, ToJolt(force));
}

void JoltPhysicsWorld::AddImpulse(UUID entity, const glm::vec3& impulse) {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return;
    }
    m_System->GetBodyInterface().AddImpulse(id, ToJolt(impulse));
}

glm::vec3 JoltPhysicsWorld::GetLinearVelocity(UUID entity) const {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return glm::vec3(0.0f);
    }
    return ToGlm(m_System->GetBodyInterface().GetLinearVelocity(id));
}

glm::vec3 JoltPhysicsWorld::GetAngularVelocity(UUID entity) const {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return glm::vec3(0.0f);
    }
    return ToGlm(m_System->GetBodyInterface().GetAngularVelocity(id));
}

bool JoltPhysicsWorld::GetBodyPosition(UUID entity, glm::vec3& outPosition) const {
    JPH::BodyID id;
    if (!FindBody(entity, id)) {
        return false;
    }
    outPosition = RToGlm(m_System->GetBodyInterface().GetPosition(id));
    return true;
}

// --- contact / trigger events (Step 7) --------------------------------------

std::vector<PhysicsContactEvent> JoltPhysicsWorld::DrainContactEvents() {
    if (!m_ContactListener) {
        return {};
    }
    return m_ContactListener->DrainContacts();
}

std::vector<PhysicsTriggerEvent> JoltPhysicsWorld::DrainTriggerEvents() {
    if (!m_ContactListener) {
        return {};
    }
    return m_ContactListener->DrainTriggers();
}

bool JoltPhysicsWorld::ResolveEntity(const JPH::BodyID& bodyId, UUID& outEntity) const {
    const auto it = m_BodyToEntity.find(bodyId);
    if (it == m_BodyToEntity.end()) {
        return false;
    }
    outEntity = it->second;
    return true;
}

// Query methods (Step 8) live in JoltQueries.cpp.
// CollectDebugDraw (Step 9) lives in JoltDebugRenderer.cpp.

} // namespace Hockey::JoltDetail
