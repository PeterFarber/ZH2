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
    input.sprint = true;
    input.shootPressed = true;
    input.passReleased = true;

    GameplayInputBuffer buffer;
    buffer.PushInput(input);
    HK_CHECK_EQ(buffer.Size(), static_cast<std::size_t>(1));

    GameplayInputFrame out;
    HK_CHECK(buffer.GetInput(2, out));
    HK_CHECK_EQ(out.inputSequence, 10ull);
    HK_CHECK_EQ(out.simulationTick, 42ull);
    HK_CHECK_NEAR(out.move.y, -1.0f, 0.0001f);
    HK_CHECK(out.sprint);
    HK_CHECK(out.shootPressed);
    HK_CHECK(out.passReleased);

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
}
