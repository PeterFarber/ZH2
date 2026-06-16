#include "Test.hpp"

#include "Hockey/Gameplay/GameplayTypes.hpp"
#include "Hockey/Gameplay/Teams/TeamTypes.hpp"

using namespace Hockey;

void RunTeamTypesTests() {
    HockeyTest::BeginSuite("TeamTypesTests");

    HK_CHECK_EQ(OppositeTeam(GameplayTeam::Home), GameplayTeam::Away);
    HK_CHECK_EQ(OppositeTeam(GameplayTeam::Away), GameplayTeam::Home);
    HK_CHECK_EQ(OppositeTeam(GameplayTeam::None), GameplayTeam::None);

    HK_CHECK(IsHomeSlot(PlayerSlot::HomeSkater0));
    HK_CHECK(IsHomeSlot(PlayerSlot::HomeGoalie));
    HK_CHECK(IsAwaySlot(PlayerSlot::AwaySkater2));
    HK_CHECK(IsAwaySlot(PlayerSlot::AwayGoalie));
    HK_CHECK(IsSkaterSlot(PlayerSlot::HomeSkater1));
    HK_CHECK(IsSkaterSlot(PlayerSlot::AwaySkater1));
    HK_CHECK(IsGoalieSlot(PlayerSlot::HomeGoalie));
    HK_CHECK(IsGoalieSlot(PlayerSlot::AwayGoalie));
    HK_CHECK(!IsSkaterSlot(PlayerSlot::None));

    GameplayTeam team = GameplayTeam::None;
    HK_CHECK(GameplayTeamFromString("Home", team));
    HK_CHECK_EQ(team, GameplayTeam::Home);

    GameplayRole role = GameplayRole::Skater;
    HK_CHECK(GameplayRoleFromString("Goalie", role));
    HK_CHECK_EQ(role, GameplayRole::Goalie);

    PlayerSlot slot = PlayerSlot::None;
    HK_CHECK(PlayerSlotFromString("AwayGoalie", slot));
    HK_CHECK_EQ(slot, PlayerSlot::AwayGoalie);

    MatchPhase phase = MatchPhase::NotStarted;
    HK_CHECK(MatchPhaseFromString("Playing", phase));
    HK_CHECK_EQ(phase, MatchPhase::Playing);
    HK_CHECK_EQ(std::string(MatchPhaseToString(MatchPhase::GoalScored)), std::string("GoalScored"));
}
