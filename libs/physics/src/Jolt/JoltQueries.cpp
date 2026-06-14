#include "Hockey/Physics/Jolt/JoltPhysicsWorld.hpp"

#include <unordered_set>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "Hockey/Physics/Jolt/JoltTypeConversions.hpp"

namespace Hockey::JoltDetail {

namespace {

// Accepts a body only if its object layer's bit is set in the request mask.
class LayerMaskFilter final : public JPH::ObjectLayerFilter {
public:
    explicit LayerMaskFilter(std::uint32_t mask) : m_Mask(mask) {}

    bool ShouldCollide(JPH::ObjectLayer layer) const override {
        if (layer >= 32u) {
            return false;
        }
        return (m_Mask & (1u << layer)) != 0u;
    }

private:
    std::uint32_t m_Mask;
};

// Rejects sensor (trigger) bodies unless the query opted into triggers.
class TriggerBodyFilter final : public JPH::BodyFilter {
public:
    explicit TriggerBodyFilter(bool includeTriggers) : m_IncludeTriggers(includeTriggers) {}

    bool ShouldCollideLocked(const JPH::Body& body) const override {
        return m_IncludeTriggers || !body.IsSensor();
    }

private:
    bool m_IncludeTriggers;
};

bool NormalizedDirection(const glm::vec3& in, glm::vec3& out) {
    const float length = glm::length(in);
    if (length < 1e-6f) {
        return false;
    }
    out = in / length;
    return true;
}

} // namespace

bool JoltPhysicsWorld::Raycast(const RaycastRequest& request, RaycastHit& outHit) const {
    if (!m_Initialized) {
        return false;
    }
    glm::vec3 direction;
    if (!NormalizedDirection(request.direction, direction)) {
        return false;
    }

    const JPH::RRayCast ray(ToJoltR(request.origin), ToJolt(direction * request.maxDistance));
    JPH::RayCastResult result;

    const LayerMaskFilter layerFilter(request.layerMask);
    const TriggerBodyFilter bodyFilter(request.includeTriggers);

    if (!m_System->GetNarrowPhaseQuery().CastRay(ray, result, {}, layerFilter, bodyFilter)) {
        return false;
    }

    const glm::vec3 point = request.origin + direction * (request.maxDistance * result.mFraction);
    ResolveEntity(result.mBodyID, outHit.entity);
    outHit.point = point;
    outHit.distance = request.maxDistance * result.mFraction;

    JPH::BodyLockRead lock(m_System->GetBodyLockInterface(), result.mBodyID);
    if (lock.Succeeded()) {
        outHit.normal = ToGlm(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ToJoltR(point)));
    }
    return true;
}

std::vector<RaycastHit> JoltPhysicsWorld::RaycastAll(const RaycastRequest& request) const {
    std::vector<RaycastHit> hits;
    if (!m_Initialized) {
        return hits;
    }
    glm::vec3 direction;
    if (!NormalizedDirection(request.direction, direction)) {
        return hits;
    }

    const JPH::RRayCast ray(ToJoltR(request.origin), ToJolt(direction * request.maxDistance));
    JPH::RayCastSettings settings;

    const LayerMaskFilter layerFilter(request.layerMask);
    const TriggerBodyFilter bodyFilter(request.includeTriggers);

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    m_System->GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, layerFilter, bodyFilter);
    collector.Sort();

    const JPH::BodyLockInterfaceLocking& lockInterface = m_System->GetBodyLockInterface();
    for (const JPH::RayCastResult& result : collector.mHits) {
        RaycastHit hit;
        const glm::vec3 point = request.origin + direction * (request.maxDistance * result.mFraction);
        ResolveEntity(result.mBodyID, hit.entity);
        hit.point = point;
        hit.distance = request.maxDistance * result.mFraction;

        JPH::BodyLockRead lock(lockInterface, result.mBodyID);
        if (lock.Succeeded()) {
            hit.normal = ToGlm(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ToJoltR(point)));
        }
        hits.push_back(hit);
    }
    return hits;
}

bool JoltPhysicsWorld::OverlapSphere(const OverlapSphereRequest& request, std::vector<OverlapHit>& outHits) const {
    outHits.clear();
    if (!m_Initialized || request.radius <= 0.0f) {
        return false;
    }

    JPH::SphereShapeSettings shapeSettings(request.radius);
    const JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        return false;
    }

    const JPH::CollideShapeSettings settings;
    const LayerMaskFilter layerFilter(request.layerMask);
    const TriggerBodyFilter bodyFilter(request.includeTriggers);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    m_System->GetNarrowPhaseQuery().CollideShape(shapeResult.Get(), JPH::Vec3::sReplicate(1.0f),
                                                 JPH::RMat44::sTranslation(ToJoltR(request.center)), settings,
                                                 ToJoltR(request.center), collector, {}, layerFilter, bodyFilter);

    std::unordered_set<JPH::BodyID, BodyIDHash> seen;
    for (const JPH::CollideShapeResult& result : collector.mHits) {
        if (!seen.insert(result.mBodyID2).second) {
            continue;
        }
        OverlapHit hit;
        ResolveEntity(result.mBodyID2, hit.entity);
        outHits.push_back(hit);
    }
    return !outHits.empty();
}

bool JoltPhysicsWorld::OverlapBox(const OverlapBoxRequest& request, std::vector<OverlapHit>& outHits) const {
    outHits.clear();
    if (!m_Initialized || request.halfExtents.x <= 0.0f || request.halfExtents.y <= 0.0f ||
        request.halfExtents.z <= 0.0f) {
        return false;
    }

    JPH::BoxShapeSettings shapeSettings(ToJolt(request.halfExtents), 0.0f);
    const JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError()) {
        return false;
    }

    const JPH::CollideShapeSettings settings;
    const LayerMaskFilter layerFilter(request.layerMask);
    const TriggerBodyFilter bodyFilter(request.includeTriggers);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    m_System->GetNarrowPhaseQuery().CollideShape(
        shapeResult.Get(), JPH::Vec3::sReplicate(1.0f),
        JPH::RMat44::sRotationTranslation(ToJolt(request.rotation), ToJoltR(request.center)), settings,
        ToJoltR(request.center), collector, {}, layerFilter, bodyFilter);

    std::unordered_set<JPH::BodyID, BodyIDHash> seen;
    for (const JPH::CollideShapeResult& result : collector.mHits) {
        if (!seen.insert(result.mBodyID2).second) {
            continue;
        }
        OverlapHit hit;
        ResolveEntity(result.mBodyID2, hit.entity);
        outHits.push_back(hit);
    }
    return !outHits.empty();
}

} // namespace Hockey::JoltDetail
