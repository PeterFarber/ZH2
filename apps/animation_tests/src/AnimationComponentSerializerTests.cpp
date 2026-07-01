#include "Test.hpp"

#include "Hockey/Animation/AnimationComponents.hpp"
#include "Hockey/Animation/AnimationSerializer.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"

using namespace Hockey;

void RunAnimationComponentSerializerTests() {
    HockeyTest::BeginSuite("AnimationComponentSerializer");

    {
        AnimatorComponent animator;
        HK_CHECK_EQ(animator.animationGraphAsset, 0ull);
        HK_CHECK_EQ(animator.currentState, std::string{});
        HK_CHECK_NEAR(animator.playbackTimeSeconds, 0.0f, 0.0001f);
        HK_CHECK_NEAR(animator.playbackSpeed, 1.0f, 0.0001f);
        HK_CHECK(animator.playing);

        AnimationParameterComponent parameters;
        HK_CHECK_NEAR(parameters.speed, 0.0f, 0.0001f);
        HK_CHECK_NEAR(parameters.shotChargeRatio, 0.0f, 0.0001f);
        HK_CHECK(!parameters.hasPuck);
        HK_CHECK(!parameters.shooting);
        HK_CHECK(!parameters.stealing);
        HK_CHECK(!parameters.goalieAction);
        HK_CHECK(!parameters.celebrating);
    }

    ComponentSerializer::ClearExternal();
    RegisterAnimationComponentSerialization();

    Scene scene("AnimationComponents");
    Entity player = scene.CreateEntity("Animated Player");

    AnimatorComponent animator;
    animator.animationGraphAsset = 123456ull;
    animator.currentState = "SkateForward";
    animator.playbackTimeSeconds = 0.25f;
    animator.playbackSpeed = 1.5f;
    animator.playing = false;
    player.AddComponent<AnimatorComponent>(animator);

    AnimationParameterComponent parameters;
    parameters.speed = 6.75f;
    parameters.shotChargeRatio = 0.8f;
    parameters.hasPuck = true;
    parameters.shooting = true;
    parameters.stealing = false;
    parameters.goalieAction = true;
    parameters.celebrating = true;
    player.AddComponent<AnimationParameterComponent>(parameters);

    const std::filesystem::path path = Paths::TempFile("animation_components.scene.yaml");
    HK_CHECK(static_cast<bool>(SceneSerializer(scene).Serialize(path)));

    Scene loaded("LoadedAnimationComponents");
    HK_CHECK(static_cast<bool>(SceneSerializer(loaded).Deserialize(path)));

    Entity loadedPlayer = loaded.FindEntityByUUID(player.GetUUID());
    HK_CHECK(loadedPlayer.IsValid());
    HK_CHECK(loadedPlayer.HasComponent<AnimatorComponent>());
    if (loadedPlayer.HasComponent<AnimatorComponent>()) {
        const AnimatorComponent& loadedAnimator = loadedPlayer.GetComponent<AnimatorComponent>();
        HK_CHECK_EQ(loadedAnimator.animationGraphAsset, 123456ull);
        HK_CHECK_EQ(loadedAnimator.currentState, std::string("SkateForward"));
        HK_CHECK_NEAR(loadedAnimator.playbackTimeSeconds, 0.25f, 0.0001f);
        HK_CHECK_NEAR(loadedAnimator.playbackSpeed, 1.5f, 0.0001f);
        HK_CHECK(!loadedAnimator.playing);
    }

    HK_CHECK(loadedPlayer.HasComponent<AnimationParameterComponent>());
    if (loadedPlayer.HasComponent<AnimationParameterComponent>()) {
        const AnimationParameterComponent& loadedParameters = loadedPlayer.GetComponent<AnimationParameterComponent>();
        HK_CHECK_NEAR(loadedParameters.speed, 6.75f, 0.0001f);
        HK_CHECK_NEAR(loadedParameters.shotChargeRatio, 0.8f, 0.0001f);
        HK_CHECK(loadedParameters.hasPuck);
        HK_CHECK(loadedParameters.shooting);
        HK_CHECK(!loadedParameters.stealing);
        HK_CHECK(loadedParameters.goalieAction);
        HK_CHECK(loadedParameters.celebrating);
    }

    ComponentSerializer::ClearExternal();
}
