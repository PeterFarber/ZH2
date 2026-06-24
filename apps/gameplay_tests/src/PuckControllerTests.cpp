#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckController.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"

using namespace Hockey;

namespace {

Entity AddPuck(Scene& scene, PuckState state, const glm::vec3& velocity) {
    Entity puck = scene.CreateEntity("Puck");
    puck.AddComponent<PuckComponent>();
    puck.AddComponent<PuckGameplayComponent>().state = state;
    puck.AddComponent<PuckRuntimeComponent>().velocity = velocity;
    return puck;
}

Entity AddPlayer(Scene& scene) {
    Entity player = scene.CreateEntity("Player");
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 0;
    playerComponent.team = GameplayTeam::Home;
    playerComponent.slot = PlayerSlot::HomeSkater0;
    player.AddComponent<PlayerComponent>(playerComponent);
    player.AddComponent<SkaterComponent>();
    player.AddComponent<PlayerRuntimeComponent>();
    StickComponent stick;
    stick.ownerPlayer = player.GetUUID();
    player.AddComponent<StickComponent>(stick);
    return player;
}

} // namespace

void RunPuckControllerTests() {
    HockeyTest::BeginSuite("PuckControllerTests");

    {
        Scene scene("PuckControllerTravel");
        Entity puck = AddPuck(scene, PuckState::Shot, {10.0f, 0.0f, 0.0f});
        GameplayTuning tuning;
        tuning.puck.loosePuckDrag = 0.0f;

        PuckController::FixedUpdate(scene, tuning, 0.5f);

        HK_CHECK_NEAR(puck.GetComponent<TransformComponent>().localPosition.x, 5.0f, 0.0001f);
        HK_CHECK_NEAR(puck.GetComponent<PuckRuntimeComponent>().velocity.x, 10.0f, 0.0001f);
        HK_CHECK_NEAR(puck.GetComponent<PuckGameplayComponent>().timeSinceLastTouch, 0.5f, 0.0001f);
    }

    {
        Scene scene("PuckControllerDrag");
        Entity puck = AddPuck(scene, PuckState::Loose, {4.0f, 0.0f, 0.0f});
        GameplayTuning tuning;
        tuning.puck.loosePuckDrag = 0.5f;

        PuckController::FixedUpdate(scene, tuning, 1.0f);

        HK_CHECK_NEAR(puck.GetComponent<PuckRuntimeComponent>().velocity.x, 2.0f, 0.0001f);
        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
    }

    {
        Scene scene("PuckControllerFloorClamp");
        Entity puck = AddPuck(scene, PuckState::Shot, {0.0f, -3.0f, 0.0f});
        puck.GetComponent<TransformComponent>().localPosition = {0.0f, -0.25f, 0.0f};
        GameplayTuning tuning;
        tuning.puck.floorY = 0.05f;

        PuckController::FixedUpdate(scene, tuning, 0.25f);

        HK_CHECK_NEAR(puck.GetComponent<TransformComponent>().localPosition.y, tuning.puck.floorY, 0.0001f);
        HK_CHECK_NEAR(puck.GetComponent<PuckRuntimeComponent>().velocity.y, 0.0f, 0.0001f);
        HK_CHECK_NEAR(puck.GetComponent<PuckRuntimeComponent>().targetPosition.y, tuning.puck.floorY, 0.0001f);
    }

    {
        Scene scene("MovingPuckPossession");
        Entity player = AddPlayer(scene);
        Entity puck = AddPuck(scene, PuckState::Shot, {1.0f, 0.0f, 0.0f});
        player.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
        puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(player);

        GameplayEventQueue events;
        HK_CHECK(PuckPossession::TryAcquire(scene, player, puck, events));
        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
        HK_CHECK_EQ(puck.GetComponent<PuckRuntimeComponent>().velocity, glm::vec3{0.0f});
    }
}
