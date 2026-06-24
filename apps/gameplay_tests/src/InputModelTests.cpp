#include "Test.hpp"

#include <type_traits>

#include "Hockey/Gameplay/GameplayInput.hpp"

using namespace Hockey;

namespace {

template <typename T, typename = void>
struct HasPassPressed : std::false_type {};
template <typename T>
struct HasPassPressed<T, std::void_t<decltype(&T::passPressed)>> : std::true_type {};

template <typename T, typename = void>
struct HasCheckPressed : std::false_type {};
template <typename T>
struct HasCheckPressed<T, std::void_t<decltype(&T::checkPressed)>> : std::true_type {};

template <typename T, typename = void>
struct HasPokeCheckPressed : std::false_type {};
template <typename T>
struct HasPokeCheckPressed<T, std::void_t<decltype(&T::pokeCheckPressed)>> : std::true_type {};

template <typename T, typename = void>
struct HasStealHeld : std::false_type {};
template <typename T>
struct HasStealHeld<T, std::void_t<decltype(&T::stealHeld)>> : std::true_type {};

template <typename T, typename = void>
struct HasStealReleased : std::false_type {};
template <typename T>
struct HasStealReleased<T, std::void_t<decltype(&T::stealReleased)>> : std::true_type {};

} // namespace

void RunInputModelTests() {
    HockeyTest::BeginSuite("InputModelTests");

    HK_CHECK(!HasPassPressed<GameplayInputFrame>::value);
    HK_CHECK(!HasCheckPressed<GameplayInputFrame>::value);
    HK_CHECK(!HasPokeCheckPressed<GameplayInputFrame>::value);
    HK_CHECK(!HasStealHeld<GameplayInputFrame>::value);
    HK_CHECK(!HasStealReleased<GameplayInputFrame>::value);

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
    input.boostPressed = true;
    input.brakePressed = true;
    input.brakeHeld = true;
    input.quickTurnPressed = true;
    input.shootPressed = true;
    input.shootHeld = true;
    input.shootReleased = true;
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
    HK_CHECK(out.boostPressed);
    HK_CHECK(out.brakePressed);
    HK_CHECK(out.brakeHeld);
    HK_CHECK(out.quickTurnPressed);
    HK_CHECK(out.shootPressed);
    HK_CHECK(out.shootHeld);
    HK_CHECK(out.shootReleased);
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

    GameplayInputFrame actionInput;
    actionInput.stealPressed = true;
    actionInput.shootPressed = true;
    actionInput.shootHeld = true;
    actionInput.shootReleased = true;
    HK_CHECK(actionInput.stealPressed);
    HK_CHECK(actionInput.shootPressed);
    HK_CHECK(actionInput.shootHeld);
    HK_CHECK(actionInput.shootReleased);
}
