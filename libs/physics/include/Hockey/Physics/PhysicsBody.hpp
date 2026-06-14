#pragma once

#include <cstdint>

namespace Hockey {

// Opaque, stable handle to a physics body owned by a PhysicsWorld. 0 == invalid.
// Internally the world maps this to a Jolt BodyID (and back to an entity UUID).
struct PhysicsBodyHandle {
    std::uint32_t id = 0;

    bool IsValid() const {
        return id != 0;
    }

    bool operator==(const PhysicsBodyHandle& other) const {
        return id == other.id;
    }
    bool operator!=(const PhysicsBodyHandle& other) const {
        return id != other.id;
    }
};

} // namespace Hockey
