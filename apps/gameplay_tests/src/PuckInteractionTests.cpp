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
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
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

glm::vec3 FloorStickPosition(Entity player, float floorY) {
    glm::vec3 position = StickHandling::GetStickWorldPosition(player);
    position.y = floorY;
    return position;
}

} // namespace

void RunStickHandlingTests() {
    HockeyTest::BeginSuite("StickHandlingTests");

    Scene scene("StickHandlingScene");
    BuildValidGameplayScene(scene);

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
    world.DrainEvents();

    Entity skater = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(skater.IsValid());
    HK_CHECK(puck.IsValid());
    HK_CHECK_EQ(skater.GetComponent<StickComponent>().ownerPlayer, skater.GetUUID());

    skater.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, 3.0f};
    skater.GetComponent<PlayerRuntimeComponent>().facingDirection = {1.0f, 0.0f, 0.0f};
    const glm::vec3 expectedStickPosition{3.0f, 0.0f, 3.0f};
    HK_CHECK_EQ(StickHandling::GetStickWorldPosition(skater), expectedStickPosition);

    puck.GetComponent<TransformComponent>().localPosition = expectedStickPosition;
    HK_CHECK(StickHandling::CanControlPuck(skater, puck));

    puck.GetComponent<TransformComponent>().localPosition = {20.0f, 0.0f, 20.0f};
    HK_CHECK(!StickHandling::CanControlPuck(skater, puck));
}

void RunPuckInteractionTests() {
    HockeyTest::BeginSuite("PuckInteractionTests");

    Scene scene("PuckInteractionScene");
    BuildValidGameplayScene(scene);
    const GameplayTuning defaultTuning;

    GameplayWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
    world.DrainEvents();

    FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;
    Entity home = FindPlayer(scene, PlayerSlot::HomeSkater0);
    Entity away = FindPlayer(scene, PlayerSlot::AwaySkater0);
    Entity puck = FindPuck(scene);
    HK_CHECK(home.IsValid());
    HK_CHECK(away.IsValid());
    HK_CHECK(puck.IsValid());

    home.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
    home.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    away.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.25f};
    away.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(home);

    GameplayEventQueue events;
    HK_CHECK(PuckPossession::TryAcquire(scene, home, puck, events));
    HK_CHECK(SawEvent(events.Drain(), GameplayEventType::PuckPossessionChanged));
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, home.GetUUID());
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().lastTouchedPlayer, home.GetUUID());
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().lastTouchedTeam, GameplayTeam::Home);
    HK_CHECK(home.GetComponent<SkaterComponent>().hasPuck);

    HK_CHECK(!PuckPossession::TryAcquire(scene, away, puck, events));
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, home.GetUUID());

    home.GetComponent<TransformComponent>().localPosition = {2.0f, 0.0f, 0.0f};
    PuckPossession::FixedUpdate(scene, events);
    HK_CHECK_EQ(puck.GetComponent<TransformComponent>().localPosition, FloorStickPosition(home, defaultTuning.puck.floorY));

    PuckPossession::Release(scene, puck, events);
    HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
    HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
    HK_CHECK(!home.GetComponent<SkaterComponent>().hasPuck);
    HK_CHECK(SawEvent(events.Drain(), GameplayEventType::PuckPossessionChanged));

    Scene bodyContactScene("PuckBodyContactAcquire");
    Entity bodyPlayer = AddMarker(bodyContactScene, "Body Player", {0.0f, 0.0f, 0.0f});
    PlayerComponent bodyPlayerComponent;
    bodyPlayerComponent.team = GameplayTeam::Home;
    bodyPlayerComponent.slot = PlayerSlot::HomeSkater0;
    bodyPlayer.AddComponent<PlayerComponent>(bodyPlayerComponent);
    bodyPlayer.AddComponent<SkaterComponent>();
    bodyPlayer.AddComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    StickComponent shortStick;
    shortStick.ownerPlayer = bodyPlayer.GetUUID();
    shortStick.reach = 0.05f;
    shortStick.localOffset = {0.0f, 0.0f, 1.0f};
    bodyPlayer.AddComponent<StickComponent>(shortStick);
    CylinderColliderComponent playerCollider;
    playerCollider.radius = 0.45f;
    playerCollider.halfHeight = 0.9f;
    bodyPlayer.AddComponent<CylinderColliderComponent>(playerCollider);

    Entity contactPuck = AddMarker(bodyContactScene, "Contact Puck", {0.7f, 0.05f, 0.0f});
    contactPuck.AddComponent<PuckComponent>();
    contactPuck.AddComponent<PuckGameplayComponent>();
    contactPuck.AddComponent<PuckRuntimeComponent>();
    CylinderColliderComponent puckCollider;
    puckCollider.radius = 0.3f;
    puckCollider.halfHeight = 0.05f;
    contactPuck.AddComponent<CylinderColliderComponent>(puckCollider);

    GameplayEventQueue contactEvents;
    HK_CHECK(!StickHandling::CanControlPuck(bodyPlayer, contactPuck));
    HK_CHECK(PuckPossession::TryAcquire(bodyContactScene, bodyPlayer, contactPuck, contactEvents));
    HK_CHECK(SawEvent(contactEvents.Drain(), GameplayEventType::PuckPossessionChanged));
    HK_CHECK_EQ(contactPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(contactPuck.GetComponent<PuckGameplayComponent>().possessingPlayer, bodyPlayer.GetUUID());
    HK_CHECK(bodyPlayer.GetComponent<SkaterComponent>().hasPuck);

    Scene scaledContactScene("ScaledPuckBodyContactAcquire");
    Entity scaledPlayer = AddMarker(scaledContactScene, "Scaled Body Player", {0.0f, 0.0f, 0.0f});
    scaledPlayer.GetComponent<TransformComponent>().localScale = {0.5f, 1.8f, 0.5f};
    scaledPlayer.AddComponent<PlayerComponent>(bodyPlayerComponent);
    scaledPlayer.AddComponent<SkaterComponent>();
    scaledPlayer.AddComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    shortStick.ownerPlayer = scaledPlayer.GetUUID();
    scaledPlayer.AddComponent<StickComponent>(shortStick);
    CylinderColliderComponent scaledPlayerCollider;
    scaledPlayerCollider.radius = 0.25f;
    scaledPlayerCollider.halfHeight = 0.9f;
    scaledPlayer.AddComponent<CylinderColliderComponent>(scaledPlayerCollider);

    Entity scaledPuck = AddMarker(scaledContactScene, "Scaled Contact Puck", {1.0f, 0.08f, 0.0f});
    scaledPuck.GetComponent<TransformComponent>().localScale = {4.0f, 4.0f, 4.0f};
    scaledPuck.AddComponent<PuckComponent>();
    scaledPuck.AddComponent<PuckGameplayComponent>();
    scaledPuck.AddComponent<PuckRuntimeComponent>();
    CylinderColliderComponent scaledPuckCollider;
    scaledPuckCollider.radius = 0.3f;
    scaledPuckCollider.halfHeight = 0.05f;
    scaledPuck.AddComponent<CylinderColliderComponent>(scaledPuckCollider);

    GameplayEventQueue scaledContactEvents;
    HK_CHECK(!StickHandling::CanControlPuck(scaledPlayer, scaledPuck));
    HK_CHECK(PuckPossession::TryAcquire(scaledContactScene, scaledPlayer, scaledPuck, scaledContactEvents));
    HK_CHECK(SawEvent(scaledContactEvents.Drain(), GameplayEventType::PuckPossessionChanged));
    HK_CHECK_EQ(scaledPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(scaledPuck.GetComponent<PuckGameplayComponent>().possessingPlayer, scaledPlayer.GetUUID());

    Scene floorContactScene("PossessedPuckFloorHeight");
    Entity floorPlayer = AddMarker(floorContactScene, "Floor Body Player", {0.0f, 1.25f, 0.0f});
    floorPlayer.AddComponent<PlayerComponent>(bodyPlayerComponent);
    floorPlayer.AddComponent<SkaterComponent>();
    floorPlayer.AddComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    shortStick.ownerPlayer = floorPlayer.GetUUID();
    floorPlayer.AddComponent<StickComponent>(shortStick);

    Entity floorPuck = AddMarker(floorContactScene, "Floor Puck", StickHandling::GetStickWorldPosition(floorPlayer));
    floorPuck.AddComponent<PuckComponent>();
    floorPuck.AddComponent<PuckGameplayComponent>();
    floorPuck.AddComponent<PuckRuntimeComponent>();

    GameplayEventQueue floorEvents;
    HK_CHECK(PuckPossession::TryAcquire(floorContactScene, floorPlayer, floorPuck, floorEvents));
    HK_CHECK_EQ(floorPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_NEAR(floorPuck.GetComponent<TransformComponent>().localPosition.y,
                  defaultTuning.puck.floorY,
                  0.0001f);

    Scene physicsContactScene("DynamicPuckPossessionPhysicsSync");
    Entity physicsPlayer = AddMarker(physicsContactScene, "Physics Body Player", {0.0f, 0.0f, 0.0f});
    physicsPlayer.AddComponent<PlayerComponent>(bodyPlayerComponent);
    physicsPlayer.AddComponent<SkaterComponent>();
    physicsPlayer.AddComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
    shortStick.ownerPlayer = physicsPlayer.GetUUID();
    physicsPlayer.AddComponent<StickComponent>(shortStick);
    physicsPlayer.AddComponent<CylinderColliderComponent>(playerCollider);

    Entity physicsPuck = AddMarker(physicsContactScene, "Physics Contact Puck", {0.7f, 0.05f, 0.0f});
    physicsPuck.AddComponent<PuckComponent>();
    physicsPuck.AddComponent<PuckGameplayComponent>();
    physicsPuck.AddComponent<PuckRuntimeComponent>();
    RigidBodyComponent puckRigidBody;
    puckRigidBody.type = RigidBodyType::Dynamic;
    puckRigidBody.useGravity = false;
    puckRigidBody.layer = PhysicsLayer::Puck;
    physicsPuck.AddComponent<RigidBodyComponent>(puckRigidBody);
    physicsPuck.AddComponent<CylinderColliderComponent>(puckCollider);

    HK_CHECK_MSG(static_cast<bool>(Physics::Init()), "physics initializes for possessed dynamic puck sync");
    PhysicsWorld physicsWorld;
    HK_CHECK_MSG(static_cast<bool>(physicsWorld.Init(MakeDefaultPhysicsSettings())),
                 "physics world initializes for possessed dynamic puck sync");
    physicsWorld.CreateBodiesFromScene(physicsContactScene);

    GameplayEventQueue physicsContactEvents;
    HK_CHECK(PuckPossession::TryAcquire(physicsContactScene,
                                        physicsPlayer,
                                        physicsPuck,
                                        physicsContactEvents,
                                        &physicsWorld));
    const glm::vec3 acquiredPosition = physicsPuck.GetComponent<TransformComponent>().localPosition;
    HK_CHECK_EQ(acquiredPosition, FloorStickPosition(physicsPlayer, defaultTuning.puck.floorY));

    physicsWorld.SyncSceneToPhysics(physicsContactScene);
    physicsWorld.Step(1.0f / 60.0f);
    physicsWorld.SyncPhysicsToScene(physicsContactScene);

    HK_CHECK_EQ(physicsPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(physicsPuck.GetComponent<PuckGameplayComponent>().possessingPlayer, physicsPlayer.GetUUID());
    HK_CHECK_EQ(physicsPuck.GetComponent<TransformComponent>().localPosition, acquiredPosition);
    physicsWorld.Shutdown();
    Physics::Shutdown();
}
