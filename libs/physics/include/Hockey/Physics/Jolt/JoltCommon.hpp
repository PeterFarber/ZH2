#pragma once

// ---------------------------------------------------------------------------
// Central Jolt include point for hockey_physics. This header (and anything that
// includes it) is INTERNAL to the library: it pulls in <Jolt/...> headers which
// are only available on hockey_physics' private include path. Public physics
// headers must never include this file, so Jolt stays an implementation detail.
// ---------------------------------------------------------------------------

// Jolt.h must be the first Jolt include; it sets up configuration + warning
// suppression for everything that follows.
#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cstddef>
#include <functional>

namespace Hockey::JoltDetail {

// Hash so JPH::BodyID can be used as an unordered_map key (BodyID already
// provides operator==). Shared by the physics world and the contact listener.
struct BodyIDHash {
    std::size_t operator()(const JPH::BodyID& id) const noexcept {
        return std::hash<std::uint32_t>{}(id.GetIndexAndSequenceNumber());
    }
};

// Installs the global Jolt allocator, trace/assert callbacks, factory and
// default type registrations. Safe to call once per process (Physics::Init
// guards re-entry). Returns false only if the factory could not be created.
bool RegisterGlobals();

// Tears down what RegisterGlobals() created. Safe to call when not registered.
void UnregisterGlobals();

bool GlobalsRegistered();

} // namespace Hockey::JoltDetail
