#include "Test.hpp"

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/MatchState.hpp"
#include "Hockey/Gameplay/Player/PlayerComponents.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

#include <glm/geometric.hpp>

using namespace Hockey;

namespace {

struct PreviewFixture {
    Scene scene{"EditorGameplayPreview"};
    EditorContext context;

    PreviewFixture() {
        context.activeScene = &scene;
        RegisterEditorTools(context.toolManager);
        context.toolManager.Activate("Hockey Rink", context);
        context.toolManager.Activate("Hockey Goals", context);
        context.toolManager.Activate("Hockey Spawns", context);
        context.toolManager.Activate("Hockey Faceoff Spots", context);
        context.toolManager.Activate("Hockey Puck", context);
        context.toolManager.Activate("Hockey Players", context);
    }
};

Entity FindPlayerByIndex(Scene& scene, std::uint32_t playerIndex) {
    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        if (player.GetComponent<PlayerComponent>().playerIndex == playerIndex) {
            return player;
        }
    }
    return {};
}

} // namespace

void RunEditorGameplayPreviewTests() {
    HockeyTest::BeginSuite("EditorGameplayPreviewTests");

    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterPhysicsComponents();
    RegisterGameplayComponents();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();
    Physics::Init();

    {
        PreviewFixture fix;
        Entity homeSkater = fix.scene.FindEntityByName("Home Skater 0");
        HK_CHECK_MSG(static_cast<bool>(homeSkater), "fixture has home skater");
        const glm::vec3 authoredPosition = homeSkater.GetComponent<TransformComponent>().localPosition;

        EditorPhysicsPreview physicsPreview;
        physicsPreview.Configure(MakeDefaultPhysicsSettings());

        EditorGameplayPreview gameplayPreview;
        GameplaySettings settings;
        settings.fixedDeltaSeconds = 1.0f / 60.0f;
        gameplayPreview.Configure(settings);

        const Status started = gameplayPreview.Start(fix.scene, physicsPreview);
        HK_CHECK_MSG(static_cast<bool>(started), "gameplay preview starts on tool-authored scene");
        HK_CHECK(gameplayPreview.IsActive());
        HK_CHECK(physicsPreview.IsActive());
        HK_CHECK_MSG(fix.scene.FindEntityByName("Match State").HasComponent<MatchStateComponent>(),
                     "preview initialized match state");

        GameplayInputFrame input;
        input.playerIndex = 0;
        input.simulationTick = 0;
        input.move = {0.0f, 1.0f};
        gameplayPreview.Start(fix.scene, physicsPreview);
        gameplayPreview.StepOnce(fix.scene, physicsPreview);

        gameplayPreview.Reset(fix.scene, physicsPreview);
        HK_CHECK(gameplayPreview.IsActive());
        HK_CHECK_MSG(fix.scene.FindEntityByName("Match State").HasComponent<MatchStateComponent>(),
                     "reset reinitializes match state");

        gameplayPreview.Stop(fix.scene, physicsPreview);
        HK_CHECK(!gameplayPreview.IsActive());
        HK_CHECK(!physicsPreview.IsActive());

        homeSkater = fix.scene.FindEntityByName("Home Skater 0");
        HK_CHECK_MSG(static_cast<bool>(homeSkater), "authored skater restored after preview stop");
        if (homeSkater) {
            const glm::vec3 restoredPosition = homeSkater.GetComponent<TransformComponent>().localPosition;
            HK_CHECK_NEAR(restoredPosition.x, authoredPosition.x, 0.0001f);
            HK_CHECK_NEAR(restoredPosition.y, authoredPosition.y, 0.0001f);
            HK_CHECK_NEAR(restoredPosition.z, authoredPosition.z, 0.0001f);
        }
        HK_CHECK_MSG(!fix.scene.FindEntityByName("Match State"), "preview runtime match state was removed");
    }

    {
        PreviewFixture fix;
        Entity camera = fix.scene.CreateEntity("Follow Camera");
        camera.GetComponent<TransformComponent>().localPosition = {100.0f, 20.0f, 100.0f};
        CameraComponent cameraComponent;
        cameraComponent.primary = true;
        cameraComponent.followPlayer = true;
        cameraComponent.followOffset = {2.0f, 6.0f, -8.0f};
        cameraComponent.followRotation = {-20.0f, 15.0f, 0.0f};
        camera.AddComponent<CameraComponent>(cameraComponent);

        EditorPhysicsPreview physicsPreview;
        physicsPreview.Configure(MakeDefaultPhysicsSettings());

        EditorGameplayPreview gameplayPreview;
        GameplaySettings settings;
        settings.fixedDeltaSeconds = 1.0f / 60.0f;
        gameplayPreview.Configure(settings);

        const Status started = gameplayPreview.Start(fix.scene, physicsPreview);
        HK_CHECK_MSG(static_cast<bool>(started), "gameplay preview starts for follow camera");
        gameplayPreview.Play();
        gameplayPreview.Update(fix.scene, physicsPreview, 1.0f / 60.0f);

        Entity player = FindPlayerByIndex(fix.scene, 0);
        HK_CHECK_MSG(static_cast<bool>(player), "preview has player index 0");
        if (player) {
            const glm::vec3 targetPosition = glm::vec3(fix.scene.GetWorldTransform(player)[3]);
            const glm::vec3 cameraPosition = glm::vec3(fix.scene.GetWorldTransform(camera)[3]);
            HK_CHECK_NEAR(cameraPosition.x, targetPosition.x + 2.0f, 0.0001f);
            HK_CHECK_NEAR(cameraPosition.y, targetPosition.y + 6.0f, 0.0001f);
            HK_CHECK_NEAR(cameraPosition.z, targetPosition.z - 8.0f, 0.0001f);
            const glm::vec3 cameraRotation = camera.GetComponent<TransformComponent>().localRotation;
            HK_CHECK_NEAR(cameraRotation.x, -20.0f, 0.001f);
            HK_CHECK_NEAR(cameraRotation.y, 15.0f, 0.001f);

            for (const entt::entity handle : fix.scene.Registry().view<MatchStateComponent>()) {
                fix.scene.Registry().get<MatchStateComponent>(handle).phase = MatchPhase::Playing;
            }

            const glm::vec3 playerBeforeMove = glm::vec3(fix.scene.GetWorldTransform(player)[3]);
            gameplayPreview.SetInputEnabled(true);
            gameplayPreview.SetMoveTarget(playerBeforeMove + glm::vec3{20.0f, 0.0f, 0.0f});
            gameplayPreview.Update(fix.scene, physicsPreview, 1.0f / 60.0f);

            const glm::vec3 playerAfterFixedStep = glm::vec3(fix.scene.GetWorldTransform(player)[3]);
            HK_CHECK_MSG(glm::length(playerAfterFixedStep - playerBeforeMove) > 0.0001f,
                         "preview player advances during fixed gameplay tick");

            const glm::vec3 cameraAfterFixedStep = glm::vec3(fix.scene.GetWorldTransform(camera)[3]);
            gameplayPreview.Update(fix.scene, physicsPreview, 1.0f / 240.0f);
            const glm::vec3 cameraAfterRenderOnlyFrame = glm::vec3(fix.scene.GetWorldTransform(camera)[3]);

            HK_CHECK_NEAR(cameraAfterRenderOnlyFrame.x, cameraAfterFixedStep.x, 0.0001f);
            HK_CHECK_NEAR(cameraAfterRenderOnlyFrame.y, cameraAfterFixedStep.y, 0.0001f);
            HK_CHECK_NEAR(cameraAfterRenderOnlyFrame.z, cameraAfterFixedStep.z, 0.0001f);
        }

        gameplayPreview.Stop(fix.scene, physicsPreview);
    }

    Physics::Shutdown();
}
