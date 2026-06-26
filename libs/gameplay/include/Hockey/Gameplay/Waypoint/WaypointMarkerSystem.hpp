#pragma once

#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class Scene;

class WaypointMarkerSystem {
public:
    static void FixedUpdate(Scene& scene, const GameplaySettings& settings);
};

} // namespace Hockey
