#include "Test.hpp"

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/Animation/HockeyAnimationController.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

namespace {

Entity AddAnimatedPlayer(Scene& scene, const char* name) {
    Entity entity = scene.CreateEntity(name);
    entity.AddComponent<AnimatorComponent>();
    entity.AddComponent<AnimationParameterComponent>();
    return entity;
}

} // namespace

void RunHockeyAnimationControllerTests() {
    HockeyTest::BeginSuite("HockeyAnimationController");

    Scene scene("HockeyAnimationController");
    Entity skater = AddAnimatedPlayer(scene, "Skater");
    Entity goalie = AddAnimatedPlayer(scene, "Goalie");

    HockeyAnimationFrame frame;
    HockeyAnimationPlayerState skaterState;
    skaterState.entity = skater.GetUUID();
    skaterState.velocity = {3.0f, 0.0f, 4.0f};
    skaterState.hasPuck = true;
    skaterState.shotCharging = true;
    skaterState.shotChargeRatio = 0.65f;
    frame.players.push_back(skaterState);

    HockeyAnimationPlayerState goalieState;
    goalieState.entity = goalie.GetUUID();
    goalieState.velocity = {0.0f, 0.0f, 2.0f};
    goalieState.goalie = true;
    frame.players.push_back(goalieState);

    frame.triggers.push_back({skater.GetUUID(), HockeyAnimationTriggerType::Shoot});
    frame.triggers.push_back({skater.GetUUID(), HockeyAnimationTriggerType::Steal});
    frame.triggers.push_back({goalie.GetUUID(), HockeyAnimationTriggerType::GoalieSave});
    frame.triggers.push_back({skater.GetUUID(), HockeyAnimationTriggerType::Celebrate});

    HockeyAnimationController controller;
    controller.Apply(scene, frame);

    const AnimationParameterComponent& skaterParameters = skater.GetComponent<AnimationParameterComponent>();
    HK_CHECK_NEAR(skaterParameters.speed, 5.0f, 0.0001f);
    HK_CHECK(skaterParameters.hasPuck);
    HK_CHECK(skaterParameters.shooting);
    HK_CHECK(skaterParameters.stealing);
    HK_CHECK(!skaterParameters.goalieAction);
    HK_CHECK(skaterParameters.celebrating);
    HK_CHECK_NEAR(skaterParameters.shotChargeRatio, 0.65f, 0.0001f);

    const AnimationParameterComponent& goalieParameters = goalie.GetComponent<AnimationParameterComponent>();
    HK_CHECK_NEAR(goalieParameters.speed, 2.0f, 0.0001f);
    HK_CHECK(goalieParameters.goalieAction);

    AnimatorComponent& animator = skater.GetComponent<AnimatorComponent>();
    animator.currentState = "Idle";
    animator.playbackTimeSeconds = 0.5f;
    controller.Apply(scene, HockeyAnimationFrame{});
    const AnimationParameterComponent& clearedParameters = skater.GetComponent<AnimationParameterComponent>();
    HK_CHECK_NEAR(clearedParameters.speed, 0.0f, 0.0001f);
    HK_CHECK(!clearedParameters.hasPuck);
    HK_CHECK(!clearedParameters.shooting);
    HK_CHECK(!clearedParameters.stealing);
    HK_CHECK(!clearedParameters.celebrating);
    HK_CHECK_EQ(animator.currentState, std::string("Idle"));
    HK_CHECK_NEAR(animator.playbackTimeSeconds, 0.5f, 0.0001f);
}
