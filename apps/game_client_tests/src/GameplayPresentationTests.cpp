#include "Test.hpp"

#include "GameplayPresentation.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/Player/PlayerComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckComponents.hpp"

#include <glm/vec3.hpp>

namespace {

using Hockey::Entity;
using Hockey::PlayerComponent;
using Hockey::PuckComponent;
using Hockey::Scene;
using Hockey::TransformComponent;

Entity AddPlayer(Scene& scene, const glm::vec3& position) {
    Entity player = scene.CreateEntity("Player");
    player.GetComponent<TransformComponent>().localPosition = position;
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 0;
    player.AddComponent<PlayerComponent>(playerComponent);
    return player;
}

void TestPresentationInterpolatesAndRestoresPlayer() {
    Scene scene("PresentationScene");
    Entity player = AddPlayer(scene, {0.0f, 0.0f, 0.0f});

    HockeyClient::GameplayPresentationState state;
    state.CaptureFixedStep(scene);

    player.GetComponent<TransformComponent>().localPosition = {10.0f, 0.0f, 0.0f};
    state.CaptureFixedStep(scene);

    {
        HockeyClient::GameplayPresentationSettings settings;
        HockeyClient::ScopedGameplayPresentationTransforms scope(scene, state, 0.25f, settings);
        const glm::vec3 presented = player.GetComponent<TransformComponent>().localPosition;
        HK_CHECK_NEAR(presented.x, 2.5f, 0.001f);
        HK_CHECK_NEAR(presented.y, 0.0f, 0.001f);
        HK_CHECK_NEAR(presented.z, 0.0f, 0.001f);
    }

    const glm::vec3 restored = player.GetComponent<TransformComponent>().localPosition;
    HK_CHECK_NEAR(restored.x, 10.0f, 0.001f);
    HK_CHECK_NEAR(restored.y, 0.0f, 0.001f);
    HK_CHECK_NEAR(restored.z, 0.0f, 0.001f);
}

void TestPresentationCanInterpolatePuck() {
    Scene scene("PuckPresentationScene");
    Entity puck = scene.CreateEntity("Puck");
    puck.AddComponent<PuckComponent>();
    puck.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};

    HockeyClient::GameplayPresentationState state;
    state.CaptureFixedStep(scene);

    puck.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 8.0f};
    state.CaptureFixedStep(scene);

    HockeyClient::GameplayPresentationSettings settings;
    settings.interpolatePuck = true;
    {
        HockeyClient::ScopedGameplayPresentationTransforms scope(scene, state, 0.5f, settings);
        HK_CHECK_NEAR(puck.GetComponent<TransformComponent>().localPosition.z, 4.0f, 0.001f);
    }
    HK_CHECK_NEAR(puck.GetComponent<TransformComponent>().localPosition.z, 8.0f, 0.001f);
}

void TestPresentationExposesCapturedSamplesForDiagnostics() {
    Scene scene("PresentationDiagnosticsScene");
    Entity player = AddPlayer(scene, {1.0f, 0.0f, 0.0f});

    HockeyClient::GameplayPresentationState state;
    state.CaptureFixedStep(scene);
    player.GetComponent<TransformComponent>().localPosition = {4.0f, 0.0f, 0.0f};
    state.CaptureFixedStep(scene);

    HockeyClient::GameplayPresentationState::Sample sample;
    HK_CHECK(state.GetSample(player.GetUUID(), sample));
    HK_CHECK(sample.initialized);
    HK_CHECK_NEAR(sample.previousPosition.x, 1.0f, 0.001f);
    HK_CHECK_NEAR(sample.currentPosition.x, 4.0f, 0.001f);
}

} // namespace

void RunGameplayPresentationTests() {
    HockeyTest::BeginSuite("GameplayPresentation");
    TestPresentationInterpolatesAndRestoresPlayer();
    TestPresentationCanInterpolatePuck();
    TestPresentationExposesCapturedSamplesForDiagnostics();
}
