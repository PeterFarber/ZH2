#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace Hockey {

struct WaypointMarkerComponent {
    uint32_t ownerPlayerIndex = 0;
    glm::vec3 targetPosition{0.0f};
    bool active = false;
};

template <typename T> struct ComponentTraits;

template <> struct ComponentTraits<WaypointMarkerComponent> {
    static constexpr const char* Name = "WaypointMarkerComponent";
};

} // namespace Hockey
