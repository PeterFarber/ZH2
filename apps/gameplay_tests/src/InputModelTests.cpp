#include "Test.hpp"

#include "Hockey/Gameplay/GameplayInput.hpp"

using namespace Hockey;

void RunInputModelTests() {
    HockeyTest::BeginSuite("InputModelTests");

    GameplayInputFrame input;
    input.playerIndex = 2;
    input.inputSequence = 10;
    input.simulationTick = 42;
    input.move = {0.5f, -1.0f};
    input.aim = {1.0f, 0.0f};
    input.moveTarget = {4.0f, 0.0f, 8.0f};
    input.setMoveTarget = true;
    input.clearMoveTarget = true;
    input.stealPressed = true;
    input.stealHeld = true;
    input.stealReleased = true;
    input.boostPressed = true;
    input.brakePressed = true;
    input.brakeHeld = true;
    input.quickTurnPressed = true;
    input.shootPressed = true;
    input.passReleased = true;
    input.goalieShieldPressed = true;

    GameplayInputBuffer buffer;
    buffer.PushInput(input);
    HK_CHECK_EQ(buffer.Size(), static_cast<std::size_t>(1));

    GameplayInputFrame out;
    HK_CHECK(buffer.GetInput(2, out));
    HK_CHECK_EQ(out.inputSequence, 10ull);
    HK_CHECK_EQ(out.simulationTick, 42ull);
    HK_CHECK_NEAR(out.move.y, -1.0f, 0.0001f);
    HK_CHECK(out.setMoveTarget);
    HK_CHECK(out.clearMoveTarget);
    HK_CHECK_NEAR(out.moveTarget.z, 8.0f, 0.0001f);
    HK_CHECK(out.stealPressed);
    HK_CHECK(out.stealHeld);
    HK_CHECK(out.stealReleased);
    HK_CHECK(out.boostPressed);
    HK_CHECK(out.brakePressed);
    HK_CHECK(out.brakeHeld);
    HK_CHECK(out.quickTurnPressed);
    HK_CHECK(out.shootPressed);
    HK_CHECK(out.passReleased);
    HK_CHECK(out.goalieShieldPressed);

    GameplayInputFrame older = input;
    older.inputSequence = 9;
    older.move = {0.0f, 0.0f};
    buffer.PushInput(older);
    HK_CHECK(buffer.GetInput(2, out));
    HK_CHECK_EQ(out.inputSequence, 10ull);
    HK_CHECK_NEAR(out.move.x, 0.5f, 0.0001f);

    GameplayInputFrame newer = input;
    newer.inputSequence = 11;
    newer.simulationTick = 43;
    newer.move = {1.0f, 1.0f};
    buffer.PushInput(newer);
    HK_CHECK(buffer.GetInput(2, out));
    HK_CHECK_EQ(out.inputSequence, 11ull);
    HK_CHECK_NEAR(out.move.x, 1.0f, 0.0001f);

    buffer.ClearForTick(42);
    HK_CHECK(buffer.GetInput(2, out));
    buffer.ClearForTick(43);
    HK_CHECK(!buffer.GetInput(2, out));
    HK_CHECK_EQ(buffer.Size(), static_cast<std::size_t>(0));

    buffer.PushInput(input);
    buffer.Reset();
    HK_CHECK_EQ(buffer.Size(), static_cast<std::size_t>(0));

    glm::vec2 aim{0.0f};
    HK_CHECK(TryBuildAimFromWorldTarget({2.0f, 1.0f, 3.0f}, {5.0f, 0.0f, 7.0f}, aim));
    HK_CHECK_NEAR(aim.x, 0.6f, 0.0001f);
    HK_CHECK_NEAR(aim.y, 0.8f, 0.0001f);

    aim = {0.25f, 0.5f};
    HK_CHECK(!TryBuildAimFromWorldTarget({1.0f, 0.0f, 1.0f}, {1.0f, 2.0f, 1.0f}, aim));
    HK_CHECK_NEAR(aim.x, 0.25f, 0.0001f);
    HK_CHECK_NEAR(aim.y, 0.5f, 0.0001f);
}
