#pragma once

#include <vector>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

namespace Hockey {

struct TeamStateComponent {
    GameplayTeam team = GameplayTeam::None;
    uint32_t score = 0;
    std::vector<UUID> players;
    UUID goalie;
};

template <> struct ComponentTraits<TeamStateComponent> {
    static constexpr const char* Name = "TeamStateComponent";
};

}
