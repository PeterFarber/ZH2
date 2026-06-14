#pragma once

// Internal helper for the physics debug pass. Despite the name, hockey_physics
// does NOT use Jolt's built-in DebugRenderer (to avoid ABI coupling with the
// precompiled library); instead it rebuilds collider wireframes from cached
// collider descriptions and live Jolt body transforms. Only included from
// hockey_physics .cpp files.
#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <unordered_map>

#include <Jolt/Physics/Body/BodyInterface.h>

#include "Hockey/Physics/Jolt/JoltPhysicsWorld.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"

namespace Hockey::JoltDetail {

// Appends collider wireframes + body centres for every cached body to outList,
// reading each body's current transform from the body interface.
void CollectBodyDebugDraw(const JPH::BodyInterface& bodyInterface,
                          const std::unordered_map<JPH::BodyID, BodyDebugInfo, BodyIDHash>& bodies,
                          PhysicsDebugDrawList& outList);

} // namespace Hockey::JoltDetail
