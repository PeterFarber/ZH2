#include "Test.hpp"

#include "Hockey/Gameplay/GameplayEvents.hpp"

using namespace Hockey;

void RunEventTests() {
    HockeyTest::BeginSuite("EventTests");

    GameplayEventQueue queue;
    HK_CHECK(queue.Empty());

    GameplayEvent event;
    event.type = GameplayEventType::GoalScored;
    event.team = GameplayTeam::Home;
    event.position = {1.0f, 2.0f, 3.0f};
    event.message = "home scored";
    queue.Push(event);

    HK_CHECK(!queue.Empty());
    HK_CHECK_EQ(queue.Size(), static_cast<std::size_t>(1));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::GoalScored)), std::string("GoalScored"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::CountdownTick)), std::string("CountdownTick"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::CountdownBeep)), std::string("CountdownBeep"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::StealAttempted)), std::string("StealAttempted"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::PlayerBoosted)), std::string("PlayerBoosted"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::GoalieShieldStarted)),
                std::string("GoalieShieldStarted"));
    HK_CHECK_EQ(std::string(GameplayEventTypeToString(GameplayEventType::GoalieShieldEnded)),
                std::string("GoalieShieldEnded"));

    std::vector<GameplayEvent> drained = queue.Drain();
    HK_CHECK(queue.Empty());
    HK_CHECK_EQ(drained.size(), static_cast<std::size_t>(1));
    HK_CHECK_EQ(drained.front().team, GameplayTeam::Home);
    HK_CHECK_NEAR(drained.front().position.z, 3.0f, 0.0001f);
    HK_CHECK_EQ(drained.front().message, std::string("home scored"));
}
