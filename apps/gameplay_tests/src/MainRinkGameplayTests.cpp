#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Simulation/GameplaySnapshot.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"

using namespace Hockey;

namespace {

bool HasIssueContaining(const std::vector<SceneValidationIssue>& issues, const char* text) {
    const std::string needle = text;
    for (const SceneValidationIssue& issue : issues) {
        if (issue.message.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunMainRinkGameplayTests() {
    HockeyTest::BeginSuite("MainRinkGameplayTests");

    RegisterPhysicsComponents();
    RegisterGameplayComponents();

    Scene scene{"MainRinkGameplay"};
    SceneSerializer serializer(scene);
    const std::filesystem::path scenePath = Paths::Get().rawAssets / "scenes/main_rink.scene.yaml";
    HK_CHECK_MSG(static_cast<bool>(serializer.Deserialize(scenePath)), "main_rink scene loads");
    HK_CHECK_MSG(scene.EntityCount() >= static_cast<std::size_t>(26), "main_rink has authored gameplay markers");
    HK_CHECK_MSG(!scene.FindEntityByName("GameObject"), "main_rink has no stray default editor entities");

    const std::vector<SceneValidationIssue> issues = SceneValidator::Validate(scene);
    HK_CHECK_MSG(!SceneValidator::HasErrors(issues), "main_rink gameplay validation has no errors");
    HK_CHECK_MSG(!HasIssueContaining(issues, "missing a trigger collider"), "main_rink goals have trigger setup");
    HK_CHECK_MSG(!HasIssueContaining(issues, "Puck entity is missing"), "main_rink has a puck");
    HK_CHECK_MSG(!HasIssueContaining(issues, "Puck entity is missing a RigidBody"), "main_rink puck has physics setup");

    GameplaySettings settings;
    settings.pregameCountdownSeconds = 0.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "main_rink initializes gameplay");
    HK_CHECK(world.IsInitialized());

    GameplaySnapshot snapshot = BuildGameplaySnapshot(scene, 1);
    HK_CHECK_EQ(snapshot.players.size(), static_cast<std::size_t>(8));
    for (const PlayerGameplaySnapshot& player : snapshot.players) {
        Entity playerEntity = scene.FindEntityByUUID(player.entity);
        HK_CHECK(playerEntity.IsValid());
        if (playerEntity.IsValid()) {
            HK_CHECK(playerEntity.HasComponent<MeshRendererComponent>());
            if (playerEntity.HasComponent<MeshRendererComponent>()) {
                HK_CHECK(playerEntity.GetComponent<MeshRendererComponent>().castsShadows);
            }
        }
    }
    HK_CHECK(snapshot.puck.entity.IsValid());
    HK_CHECK_EQ(snapshot.match.phase, MatchPhase::FaceoffSetup);

    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    snapshot = BuildGameplaySnapshot(scene, 2);
    HK_CHECK_EQ(snapshot.match.phase, MatchPhase::Faceoff);

    Entity homeGoal = scene.FindEntityByName("Home Goal");
    Entity awayGoal = scene.FindEntityByName("Away Goal");
    Entity puck = scene.FindEntityByName("Puck Spawn");
    HK_CHECK(homeGoal.HasComponent<GoalGameplayComponent>());
    HK_CHECK(awayGoal.HasComponent<GoalGameplayComponent>());
    HK_CHECK(puck.HasComponent<PuckGameplayComponent>());
    HK_CHECK(puck.HasComponent<PuckRuntimeComponent>());

    world.Shutdown();
}
