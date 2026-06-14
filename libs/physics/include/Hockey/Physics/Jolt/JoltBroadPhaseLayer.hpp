#pragma once

// Internal: pulls in Jolt headers. Only included from hockey_physics .cpp files.
#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace Hockey::JoltDetail {

namespace BroadPhase {
inline constexpr JPH::BroadPhaseLayer kNonMoving{0};
inline constexpr JPH::BroadPhaseLayer kMoving{1};
inline constexpr JPH::uint kNumLayers = 2;
} // namespace BroadPhase

// Maps engine object layers (== Hockey::PhysicsLayer) to broadphase layers.
class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BroadPhaseLayerInterfaceImpl();

    JPH::uint GetNumBroadPhaseLayers() const override;
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;
#endif

private:
    JPH::BroadPhaseLayer m_Mapping[16];
};

// Decides whether an object layer should test against a broadphase layer.
class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
};

// Object-vs-object filtering, delegating to Hockey::CollisionFilter.
class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override;
};

// Convenience bundle owned by a JoltPhysicsWorld for its lifetime.
struct LayerInterfaces {
    BroadPhaseLayerInterfaceImpl broadPhase;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhase;
    ObjectLayerPairFilterImpl objectPair;
};

} // namespace Hockey::JoltDetail
