#pragma once

namespace Hockey {

enum class GameplayTeam {
    None,
    Home,
    Away
};

enum class GameplayRole {
    Skater,
    Goalie
};

enum class PlayerSlot {
    HomeSkater0,
    HomeSkater1,
    HomeSkater2,
    HomeGoalie,
    AwaySkater0,
    AwaySkater1,
    AwaySkater2,
    AwayGoalie,
    None
};

GameplayTeam OppositeTeam(GameplayTeam team);
bool IsHomeSlot(PlayerSlot slot);
bool IsAwaySlot(PlayerSlot slot);
bool IsGoalieSlot(PlayerSlot slot);
bool IsSkaterSlot(PlayerSlot slot);

const char* GameplayTeamToString(GameplayTeam team);
bool GameplayTeamFromString(const char* text, GameplayTeam& outTeam);

const char* GameplayRoleToString(GameplayRole role);
bool GameplayRoleFromString(const char* text, GameplayRole& outRole);

const char* PlayerSlotToString(PlayerSlot slot);
bool PlayerSlotFromString(const char* text, PlayerSlot& outSlot);

}
