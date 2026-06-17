#include "Test.hpp"

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorGameplayPreview.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/MatchState.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"

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

    Physics::Shutdown();
}
