#pragma once

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

// A contact between two physics bodies, reported as entity UUIDs. Drained from
// the world once per tick. Safe to hold after the entities are destroyed (the
// UUIDs simply won't resolve anymore).
struct PhysicsContactEvent {
    enum class Type { Started, Persisted, Ended };

    UUID entityA;
    UUID entityB;

    glm::vec3 contactPoint{0.0f};
    glm::vec3 contactNormal{0.0f};

    float impulse = 0.0f;

    Type type = Type::Started;
};

// A body entering/leaving a trigger volume.
struct PhysicsTriggerEvent {
    enum class Type { Entered, Exited };

    UUID triggerEntity;
    UUID otherEntity;

    Type type = Type::Entered;
};

} // namespace Hockey
