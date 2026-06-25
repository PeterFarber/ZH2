#include "Test.hpp"

#include <filesystem>

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

bool HasGameplayFixtureContent(Scene& scene) {
    return scene.FindEntityByName("Puck Spawn").IsValid() && scene.FindEntityByName("Home Goal").IsValid() &&
           scene.FindEntityByName("Away Goal").IsValid();
}

} // namespace

void RunMainRinkGameplayTests() {
    HockeyTest::BeginSuite("MainRinkGameplayTests");

    RegisterPhysicsComponents();
    RegisterGameplayComponents();

    Scene scene{"MainSceneGameplay"};
    SceneSerializer serializer(scene);
    const std::filesystem::path mainScenePath = Paths::Get().rawAssets / "scenes/Main.scene.yaml";
    HK_CHECK_MSG(static_cast<bool>(serializer.Deserialize(mainScenePath)), "Main scene loads");

    bool usingMainScene = HasGameplayFixtureContent(scene);
    if (!usingMainScene) {
        scene.Clear();
        const std::filesystem::path fallbackScenePath = Paths::Get().rawAssets / "scenes/main_rink.scene.yaml";
        SceneSerializer fallbackSerializer(scene);
        const bool loadedFallback = std::filesystem::exists(fallbackScenePath) &&
                                    static_cast<bool>(fallbackSerializer.Deserialize(fallbackScenePath));
        HK_CHECK_MSG(loadedFallback, "main_rink fallback scene loads when Main is not gameplay-ready");
    }

    HK_CHECK_MSG(scene.EntityCount() >= static_cast<std::size_t>(26), "gameplay fixture has authored gameplay markers");
    HK_CHECK_MSG(!scene.FindEntityByName("GameObject"), "gameplay fixture has no stray default editor entities");

    const std::vector<SceneValidationIssue> issues = SceneValidator::Validate(scene);
    HK_CHECK_MSG(!SceneValidator::HasErrors(issues), "gameplay fixture validation has no errors");
    HK_CHECK_MSG(!HasIssueContaining(issues, "missing a trigger collider"), "gameplay fixture goals have trigger setup");
    HK_CHECK_MSG(!HasIssueContaining(issues, "Puck entity is missing"), "gameplay fixture has a puck");
    HK_CHECK_MSG(!HasIssueContaining(issues, "Puck entity is missing a RigidBody"), "gameplay fixture puck has physics setup");

    GameplaySettings settings;
    settings.pregameCountdownSeconds = 0.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay fixture initializes gameplay");
    HK_CHECK(world.IsInitialized());

    GameplaySnapshot snapshot = BuildGameplaySnapshot(scene, 1);
    HK_CHECK_EQ(snapshot.players.size(), static_cast<std::size_t>(8));
    for (const PlayerGameplaySnapshot& player : snapshot.players) {
        Entity playerEntity = scene.FindEntityByUUID(player.entity);
        HK_CHECK(playerEntity.IsValid());
        if (playerEntity.IsValid()) {
            HK_CHECK(playerEntity.HasComponent<PlayerComponent>());
            HK_CHECK(playerEntity.HasComponent<PlayerRuntimeComponent>());
            HK_CHECK(playerEntity.HasComponent<StickComponent>());
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
    HK_CHECK(puck.HasComponent<RigidBodyComponent>());
    if (usingMainScene && puck.HasComponent<RigidBodyComponent>()) {
        HK_CHECK_MSG(puck.GetComponent<RigidBodyComponent>().collisionDetection == CollisionDetectionMode::Continuous,
                     "Main scene puck uses continuous collision detection");
    }
    HK_CHECK(puck.HasComponent<PuckGameplayComponent>());
    HK_CHECK(puck.HasComponent<PuckRuntimeComponent>());

    world.Shutdown();
}
