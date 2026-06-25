#include "Test.hpp"

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

Entity AddMarker(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

void AddSpawn(Scene& scene, Team team, const glm::vec3& position, bool faceoffSpawn = false) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.faceoffSpawn = faceoffSpawn;
    spawn.AddComponent<SpawnPointComponent>(component);
}

void BuildValidGameplayScene(Scene& scene) {
    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>();
    rink.AddComponent<PlayAreaComponent>();

    AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f}).AddComponent<PuckComponent>();
    AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Home;
    AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Away;
    for (int i = 0; i < 4; ++i) {
        AddSpawn(scene, Team::Home, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, 5.0f});
    }
    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, true);
    }
}

Entity FindPlayer(Scene& scene, PlayerSlot slot) {
    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        if (player.GetComponent<PlayerComponent>().slot == slot) {
            return player;
        }
    }
    return {};
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

bool SawEvent(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

void PushShoot(GameplayWorld& world, uint32_t playerIndex, uint64_t tick, bool held, bool released) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.aim = {1.0f, 0.0f};
    input.shootHeld = held;
    input.shootReleased = released;
    world.PushInput(input);
}

void GivePuckToShooter(Scene& scene, Entity skater, Entity puck, PhysicsWorld* physicsWorld = nullptr) {
    skater.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(skater);

    GameplayEventQueue events;
    HK_CHECK(PuckPossession::TryAcquire(scene, skater, puck, events, physicsWorld));
    events.Clear();
}

} // namespace

void RunShootingTests() {
    HockeyTest::BeginSuite("ShootingTests");

    Scene scene("ShootingScene");
    BuildValidGameplayScene(scene);

    GameplaySettings settings;
    settings.periodLengthSeconds = 100.0f;
    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
    world.DrainEvents();
    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(skater.IsValid());
    HK_CHECK(puck.IsValid());

    GivePuckToShooter(scene, skater, puck);

    const uint32_t playerIndex = skater.GetComponent<PlayerComponent>().playerIndex;
    PushShoot(world, playerIndex, 1, true, false);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
    HK_CHECK(skater.GetComponent<ShotComponent>().charging);
    HK_CHECK(skater.GetComponent<ShotComponent>().charge > 0.0f);

    for (uint64_t tick = 2; tick < 90; ++tick) {
        PushShoot(world, playerIndex, tick, true, false);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, tick);
    }
    HK_CHECK_NEAR(skater.GetComponent<ShotComponent>().charge, world.GetTuning().shot.chargeSeconds, 0.001);

    PushShoot(world, playerIndex, 90, false, true);
    world.FixedUpdate(scene, settings.fixedDeltaSeconds, 90);
    std::vector<GameplayEvent> shotEvents = world.DrainEvents();

    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Shot);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
    HK_CHECK(!skater.GetComponent<SkaterComponent>().hasPuck);
    const float expectedShotSpeed =
        world.GetTuning().shot.maxPower * (1.0f - world.GetTuning().puck.loosePuckDrag * settings.fixedDeltaSeconds);
    HK_CHECK_NEAR(glm::length(puck.GetComponent<PuckRuntimeComponent>().velocity), expectedShotSpeed, 0.001);
    HK_CHECK_NEAR(glm::normalize(puck.GetComponent<PuckRuntimeComponent>().velocity).x, 1.0f, 0.001);
    HK_CHECK(!skater.GetComponent<ShotComponent>().charging);
    HK_CHECK_NEAR(skater.GetComponent<ShotComponent>().charge, 0.0f, 0.001);
    HK_CHECK(SawEvent(shotEvents, GameplayEventType::PuckShot));

    {
        Scene graceScene("ShotSelfCollisionGraceScene");
        BuildValidGameplayScene(graceScene);

        GameplayWorld graceWorld;
        HK_CHECK_MSG(static_cast<bool>(graceWorld.Init(graceScene, nullptr, settings)), "gameplay world initializes");
        graceWorld.DrainEvents();
        FindMatch(graceScene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

        Entity shooter = FindPlayer(graceScene, PlayerSlot::HomeSkater0);
        Entity gracePuck = FindPuck(graceScene);
        GivePuckToShooter(graceScene, shooter, gracePuck);

        const uint32_t shooterIndex = shooter.GetComponent<PlayerComponent>().playerIndex;
        PushShoot(graceWorld, shooterIndex, 1, true, false);
        graceWorld.FixedUpdate(graceScene, settings.fixedDeltaSeconds, 1);
        PushShoot(graceWorld, shooterIndex, 2, false, true);
        graceWorld.FixedUpdate(graceScene, settings.fixedDeltaSeconds, 2);

        PuckRuntimeComponent& runtime = gracePuck.GetComponent<PuckRuntimeComponent>();
        runtime.velocity = {0.0f, 0.0f, 0.0f};
        gracePuck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(shooter);

        graceWorld.FixedUpdate(graceScene, settings.fixedDeltaSeconds, 3);

        const PuckGameplayComponent& gameplay = gracePuck.GetComponent<PuckGameplayComponent>();
        HK_CHECK_EQ(gameplay.state, PuckState::Shot);
        HK_CHECK(!gameplay.possessingPlayer.IsValid());
        HK_CHECK(!shooter.GetComponent<SkaterComponent>().hasPuck);
    }

    {
        Scene physicsScene("DynamicPuckShotPhysicsSync");
        BuildValidGameplayScene(physicsScene);

        Entity physicsPuck = FindPuck(physicsScene);
        RigidBodyComponent puckRigidBody;
        puckRigidBody.type = RigidBodyType::Dynamic;
        puckRigidBody.useGravity = false;
        puckRigidBody.layer = PhysicsLayer::Puck;
        physicsPuck.AddComponent<RigidBodyComponent>(puckRigidBody);
        CylinderColliderComponent puckCollider;
        puckCollider.radius = 0.3f;
        puckCollider.halfHeight = 0.05f;
        physicsPuck.AddComponent<CylinderColliderComponent>(puckCollider);

        HK_CHECK_MSG(static_cast<bool>(Physics::Init()), "physics initializes for dynamic puck shot sync");
        PhysicsWorld physicsWorld;
        HK_CHECK_MSG(static_cast<bool>(physicsWorld.Init(MakeDefaultPhysicsSettings())),
                     "physics world initializes for dynamic puck shot sync");

        GameplayWorld physicsWorldGameplay;
        HK_CHECK_MSG(static_cast<bool>(physicsWorldGameplay.Init(physicsScene, &physicsWorld, settings)),
                     "gameplay world initializes");
        physicsWorldGameplay.DrainEvents();
        FindMatch(physicsScene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
        physicsWorld.CreateBodiesFromScene(physicsScene);

        Entity shooter = FindPlayer(physicsScene, PlayerSlot::HomeSkater0);
        GivePuckToShooter(physicsScene, shooter, physicsPuck, &physicsWorld);

        const uint32_t shooterIndex = shooter.GetComponent<PlayerComponent>().playerIndex;
        PushShoot(physicsWorldGameplay, shooterIndex, 1, true, false);
        physicsWorldGameplay.FixedUpdate(physicsScene, settings.fixedDeltaSeconds, 1);
        const glm::vec3 beforeRelease = physicsPuck.GetComponent<TransformComponent>().localPosition;
        PushShoot(physicsWorldGameplay, shooterIndex, 2, false, true);
        physicsWorldGameplay.FixedUpdate(physicsScene, settings.fixedDeltaSeconds, 2);

        const glm::vec3 runtimeVelocity = physicsPuck.GetComponent<PuckRuntimeComponent>().velocity;
        HK_CHECK_EQ(physicsPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Shot);
        HK_CHECK(glm::length(runtimeVelocity) > 0.0f);
        const glm::vec3 physicsVelocity = physicsWorld.GetLinearVelocity(physicsPuck);
        HK_CHECK(physicsVelocity.x > 0.0f);
        HK_CHECK_NEAR(physicsVelocity.z, 0.0f, 0.001f);

        physicsWorld.SyncSceneToPhysics(physicsScene);
        physicsWorld.Step(settings.fixedDeltaSeconds);
        physicsWorld.SyncPhysicsToScene(physicsScene);
        HK_CHECK(physicsPuck.GetComponent<TransformComponent>().localPosition.x > beforeRelease.x);

        physicsWorld.Shutdown();
        Physics::Shutdown();
    }
}
