#include "Hockey/Physics/Jolt/JoltBroadPhaseLayer.hpp"

#include "Hockey/Physics/PhysicsLayer.hpp"

namespace Hockey::JoltDetail {

namespace {

bool IsMovingLayer(PhysicsLayer layer) {
    switch (layer) {
    case PhysicsLayer::Player:
    case PhysicsLayer::Goalie:
    case PhysicsLayer::Puck:
    case PhysicsLayer::Stick:
        return true;
    default:
        return false;
    }
}

} // namespace

BroadPhaseLayerInterfaceImpl::BroadPhaseLayerInterfaceImpl() {
    for (std::uint16_t i = 0; i < 16; ++i) {
        const auto layer = static_cast<PhysicsLayer>(i);
        m_Mapping[i] = (i < kPhysicsLayerCount && IsMovingLayer(layer)) ? BroadPhase::kMoving : BroadPhase::kNonMoving;
    }
}

JPH::uint BroadPhaseLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
    return BroadPhase::kNumLayers;
}

JPH::BroadPhaseLayer BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer layer) const {
    if (layer < 16) {
        return m_Mapping[layer];
    }
    return BroadPhase::kNonMoving;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const {
    if (layer == BroadPhase::kMoving) {
        return "Moving";
    }
    return "NonMoving";
}
#endif

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer objectLayer,
                                                      JPH::BroadPhaseLayer broadPhaseLayer) const {
    const bool objectMoves = objectLayer < kPhysicsLayerCount && IsMovingLayer(static_cast<PhysicsLayer>(objectLayer));
    if (objectMoves) {
        return true;
    }
    // Non-moving objects only need to test against moving broadphase contents.
    return broadPhaseLayer == BroadPhase::kMoving;
}

bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const {
    if (a >= kPhysicsLayerCount || b >= kPhysicsLayerCount) {
        return false;
    }
    return CollisionFilter::ShouldCollide(static_cast<PhysicsLayer>(a), static_cast<PhysicsLayer>(b));
}

} // namespace Hockey::JoltDetail
