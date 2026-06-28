#include "Test.hpp"

#include "GameplayCamera.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/Player/PlayerComponents.hpp"

#include <glm/vec3.hpp>

namespace {

using Hockey::CameraComponent;
using Hockey::Entity;
using Hockey::PlayerComponent;
using Hockey::Scene;
using Hockey::TransformComponent;

void TestFollowCameraSnapsToLocalPlayer() {
    Scene scene("CameraFollowScene");
    Entity camera = scene.CreateEntity("Main Camera");
    CameraComponent cameraComponent;
    cameraComponent.primary = true;
    cameraComponent.followPlayer = true;
    cameraComponent.followOffset = {3.0f, 4.0f, 5.0f};
    cameraComponent.followRotation = {-25.0f, 10.0f, 0.0f};
    camera.AddComponent<CameraComponent>(cameraComponent);

    Entity player = scene.CreateEntity("Local Player");
    player.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, -4.0f};
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 0;
    player.AddComponent<PlayerComponent>(playerComponent);

    HockeyClient::GameplayFollowCameraSettings settings;
    settings.offset = {0.0f, 8.0f, 12.0f};
    HockeyClient::GameplayFollowCameraState state;

    HK_CHECK(HockeyClient::UpdateGameplayFollowCamera(scene, 1.0f / 60.0f, settings, state));
    const glm::vec3 cameraPosition = glm::vec3(scene.GetWorldTransform(camera)[3]);
    HK_CHECK_NEAR(cameraPosition.x, 5.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.y, 4.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.z, 1.0f, 0.0001f);
    const glm::vec3 cameraRotation = camera.GetComponent<TransformComponent>().localRotation;
    HK_CHECK_NEAR(cameraRotation.x, -25.0f, 0.001f);
    HK_CHECK_NEAR(cameraRotation.y, 10.0f, 0.001f);
}

void TestFollowCameraSmoothsAfterInitialSnap() {
    Scene scene("CameraSmoothingScene");
    Entity camera = scene.CreateEntity("Main Camera");
    CameraComponent cameraComponent;
    cameraComponent.primary = true;
    cameraComponent.followPlayer = true;
    cameraComponent.followOffset = {0.0f, 8.0f, 12.0f};
    camera.AddComponent<CameraComponent>(cameraComponent);

    Entity player = scene.CreateEntity("Local Player");
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 0;
    player.AddComponent<PlayerComponent>(playerComponent);

    HockeyClient::GameplayFollowCameraSettings settings;
    settings.offset = {0.0f, 8.0f, 12.0f};
    settings.positionResponse = 4.0f;
    HockeyClient::GameplayFollowCameraState state;

    player.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    HK_CHECK(HockeyClient::UpdateGameplayFollowCamera(scene, 1.0f / 60.0f, settings, state));

    player.GetComponent<TransformComponent>().localPosition = {10.0f, 0.0f, 0.0f};
    HK_CHECK(HockeyClient::UpdateGameplayFollowCamera(scene, 1.0f / 60.0f, settings, state));
    const glm::vec3 cameraPosition = glm::vec3(scene.GetWorldTransform(camera)[3]);
    HK_CHECK(cameraPosition.x > 0.0f);
    HK_CHECK(cameraPosition.x < 10.0f);
}

void TestFollowCameraLeavesCameraUnchangedWhenTargetMissing() {
    Scene scene("CameraMissingTargetScene");
    Entity camera = scene.CreateEntity("Main Camera");
    camera.GetComponent<TransformComponent>().localPosition = {1.0f, 2.0f, 3.0f};
    camera.AddComponent<CameraComponent>().primary = true;

    HockeyClient::GameplayFollowCameraSettings settings;
    HockeyClient::GameplayFollowCameraState state;

    HK_CHECK(!HockeyClient::UpdateGameplayFollowCamera(scene, 1.0f / 60.0f, settings, state));
    const glm::vec3 cameraPosition = glm::vec3(scene.GetWorldTransform(camera)[3]);
    HK_CHECK_NEAR(cameraPosition.x, 1.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.y, 2.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.z, 3.0f, 0.0001f);
}

void TestFollowCameraLeavesCameraUnchangedWhenActiveCameraDoesNotFollowPlayer() {
    Scene scene("CameraFollowDisabledScene");
    Entity camera = scene.CreateEntity("Main Camera");
    camera.GetComponent<TransformComponent>().localPosition = {1.0f, 2.0f, 3.0f};
    camera.AddComponent<CameraComponent>().primary = true;

    Entity player = scene.CreateEntity("Local Player");
    player.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, -4.0f};
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 0;
    player.AddComponent<PlayerComponent>(playerComponent);

    HockeyClient::GameplayFollowCameraSettings settings;
    HockeyClient::GameplayFollowCameraState state;

    HK_CHECK(!HockeyClient::UpdateGameplayFollowCamera(scene, 1.0f / 60.0f, settings, state));
    const glm::vec3 cameraPosition = glm::vec3(scene.GetWorldTransform(camera)[3]);
    HK_CHECK_NEAR(cameraPosition.x, 1.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.y, 2.0f, 0.0001f);
    HK_CHECK_NEAR(cameraPosition.z, 3.0f, 0.0001f);
}

} // namespace

void RunGameplayCameraTests() {
    HockeyTest::BeginSuite("GameplayCamera");
    TestFollowCameraSnapsToLocalPlayer();
    TestFollowCameraSmoothsAfterInitialSnap();
    TestFollowCameraLeavesCameraUnchangedWhenTargetMissing();
    TestFollowCameraLeavesCameraUnchangedWhenActiveCameraDoesNotFollowPlayer();
}
