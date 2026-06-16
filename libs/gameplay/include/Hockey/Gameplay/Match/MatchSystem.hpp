#pragma once

#include "Hockey/Core/Result.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

class MatchSystem {
public:
    static Status InitializeMatch(Scene& scene, const GameplaySettings& settings = {});
};

}
