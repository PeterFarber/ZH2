#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

#include <cstring>

namespace Hockey {

GameplayTeam OppositeTeam(GameplayTeam team) {
    if (team == GameplayTeam::Home) {
        return GameplayTeam::Away;
    }
    if (team == GameplayTeam::Away) {
        return GameplayTeam::Home;
    }
    return GameplayTeam::None;
}

bool IsHomeSlot(PlayerSlot slot) {
    return slot == PlayerSlot::HomeSkater0 || slot == PlayerSlot::HomeSkater1 || slot == PlayerSlot::HomeSkater2 ||
           slot == PlayerSlot::HomeGoalie;
}

bool IsAwaySlot(PlayerSlot slot) {
    return slot == PlayerSlot::AwaySkater0 || slot == PlayerSlot::AwaySkater1 || slot == PlayerSlot::AwaySkater2 ||
           slot == PlayerSlot::AwayGoalie;
}

bool IsGoalieSlot(PlayerSlot slot) {
    return slot == PlayerSlot::HomeGoalie || slot == PlayerSlot::AwayGoalie;
}

bool IsSkaterSlot(PlayerSlot slot) {
    return slot == PlayerSlot::HomeSkater0 || slot == PlayerSlot::HomeSkater1 || slot == PlayerSlot::HomeSkater2 ||
           slot == PlayerSlot::AwaySkater0 || slot == PlayerSlot::AwaySkater1 || slot == PlayerSlot::AwaySkater2;
}

const char* GameplayTeamToString(GameplayTeam team) {
    switch (team) {
    case GameplayTeam::None: return "None";
    case GameplayTeam::Home: return "Home";
    case GameplayTeam::Away: return "Away";
    }
    return "None";
}

bool GameplayTeamFromString(const char* text, GameplayTeam& outTeam) {
    constexpr GameplayTeam kTeams[] = {GameplayTeam::None, GameplayTeam::Home, GameplayTeam::Away};
    for (const GameplayTeam team : kTeams) {
        if (std::strcmp(text, GameplayTeamToString(team)) == 0) {
            outTeam = team;
            return true;
        }
    }
    return false;
}

const char* GameplayRoleToString(GameplayRole role) {
    switch (role) {
    case GameplayRole::Skater: return "Skater";
    case GameplayRole::Goalie: return "Goalie";
    }
    return "Skater";
}

bool GameplayRoleFromString(const char* text, GameplayRole& outRole) {
    constexpr GameplayRole kRoles[] = {GameplayRole::Skater, GameplayRole::Goalie};
    for (const GameplayRole role : kRoles) {
        if (std::strcmp(text, GameplayRoleToString(role)) == 0) {
            outRole = role;
            return true;
        }
    }
    return false;
}

const char* PlayerSlotToString(PlayerSlot slot) {
    switch (slot) {
    case PlayerSlot::HomeSkater0: return "HomeSkater0";
    case PlayerSlot::HomeSkater1: return "HomeSkater1";
    case PlayerSlot::HomeSkater2: return "HomeSkater2";
    case PlayerSlot::HomeGoalie: return "HomeGoalie";
    case PlayerSlot::AwaySkater0: return "AwaySkater0";
    case PlayerSlot::AwaySkater1: return "AwaySkater1";
    case PlayerSlot::AwaySkater2: return "AwaySkater2";
    case PlayerSlot::AwayGoalie: return "AwayGoalie";
    case PlayerSlot::None: return "None";
    }
    return "None";
}

bool PlayerSlotFromString(const char* text, PlayerSlot& outSlot) {
    constexpr PlayerSlot kSlots[] = {PlayerSlot::HomeSkater0, PlayerSlot::HomeSkater1, PlayerSlot::HomeSkater2,
                                     PlayerSlot::HomeGoalie,  PlayerSlot::AwaySkater0, PlayerSlot::AwaySkater1,
                                     PlayerSlot::AwaySkater2, PlayerSlot::AwayGoalie,  PlayerSlot::None};
    for (const PlayerSlot slot : kSlots) {
        if (std::strcmp(text, PlayerSlotToString(slot)) == 0) {
            outSlot = slot;
            return true;
        }
    }
    return false;
}

}
