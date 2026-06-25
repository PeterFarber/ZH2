#include "Test.hpp"

#include "Hockey/Physics/PhysicsLayer.hpp"

using namespace Hockey;

void RunCollisionLayerTests() {
    HockeyTest::BeginSuite("CollisionLayerTests");

    CollisionFilter::ResetDefaults();

    // Expected default pairs.
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Puck, PhysicsLayer::Rink), "puck collides with rink");
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Puck, PhysicsLayer::Goal),
                 "puck collides with goal trigger");
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Player, PhysicsLayer::Static),
                 "player collides with static");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Player, PhysicsLayer::Puck),
                 "player bodies do not physically bounce the puck");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Goalie, PhysicsLayer::Puck),
                 "goalie bodies do not physically bounce the puck");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Player, PhysicsLayer::Player),
                 "players do not collide with other players");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Player, PhysicsLayer::Goalie),
                 "skaters do not collide with goalies");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Goalie, PhysicsLayer::Goalie),
                 "goalies do not collide with other goalies");
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Stick, PhysicsLayer::Puck), "stick collides with puck");

    // Symmetry.
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Rink, PhysicsLayer::Puck), "matrix symmetric");

    // Editor collides with nothing by default.
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Editor, PhysicsLayer::Player),
                 "editor collides with nothing");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Editor, PhysicsLayer::Puck), "editor ignores puck");

    // Disabling a pair takes effect both ways.
    CollisionFilter::SetShouldCollide(PhysicsLayer::Puck, PhysicsLayer::Rink, false);
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Puck, PhysicsLayer::Rink),
                 "disabled pair does not collide");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Rink, PhysicsLayer::Puck), "disabled pair symmetric");

    // Reset restores defaults.
    CollisionFilter::ResetDefaults();
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Puck, PhysicsLayer::Rink), "reset restores default");

    // String round-trip.
    PhysicsLayer parsed = PhysicsLayer::Static;
    HK_CHECK_MSG(PhysicsLayerFromString("Puck", parsed) && parsed == PhysicsLayer::Puck, "layer parses from string");
    HK_CHECK_MSG(std::string(PhysicsLayerToString(PhysicsLayer::Goal)) == "Goal", "layer stringifies");
    HK_CHECK_MSG(!PhysicsLayerFromString("Nonsense", parsed), "invalid layer string rejected");
    HK_CHECK_MSG(IsValidPhysicsLayer(0) && IsValidPhysicsLayer(9) && !IsValidPhysicsLayer(10), "layer range check");
}
