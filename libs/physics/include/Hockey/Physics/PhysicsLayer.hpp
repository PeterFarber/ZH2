#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Hockey {

// ---------------------------------------------------------------------------
// Object layers. Each physics body belongs to exactly one layer; the collision
// matrix in CollisionFilter decides which layer pairs interact. Values map 1:1
// onto Jolt object-layer indices (cast to JPH::ObjectLayer at the boundary).
//
// The underlying type is the default (int) rather than uint16_t so the value
// round-trips safely through the editor's generic int-based enum inspector;
// only 0..9 are ever used, so the JPH::ObjectLayer (uint16) cast is lossless.
// ---------------------------------------------------------------------------
enum class PhysicsLayer {
    Static = 0,
    Player = 1,
    Goalie = 2,
    Puck = 3,
    Stick = 4,
    Rink = 5,
    Goal = 6,
    Trigger = 7,
    Sensor = 8,
    Editor = 9
};

inline constexpr std::size_t kPhysicsLayerCount = 10;

const char* PhysicsLayerToString(PhysicsLayer layer);
bool PhysicsLayerFromString(std::string_view text, PhysicsLayer& out);
bool IsValidPhysicsLayer(std::uint16_t value);

// ---------------------------------------------------------------------------
// Process-wide layer-pair collision matrix. Symmetric: enabling/disabling a
// pair affects both orderings. The defaults model a hockey rink (see plan 7.3).
// ---------------------------------------------------------------------------
class CollisionFilter {
public:
    static bool ShouldCollide(PhysicsLayer a, PhysicsLayer b);
    static void SetShouldCollide(PhysicsLayer a, PhysicsLayer b, bool shouldCollide);
    static void ResetDefaults();
};

} // namespace Hockey
