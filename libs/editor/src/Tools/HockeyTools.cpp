#include "Hockey/Editor/Tools/HockeyTools.hpp"

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/UndoRedo.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsLayer.hpp"

namespace Hockey {

namespace {

// Hockey play area is roughly 60m x 120m centred on the origin (half extents
// 30 x 60). Markers below are placed in that space; nothing here drives
// simulation, it only authors scene data.

Entity MakeEntity(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

GameplayTeam ToGameplayTeam(Team team) {
    switch (team) {
    case Team::Home: return GameplayTeam::Home;
    case Team::Away: return GameplayTeam::Away;
    case Team::None: return GameplayTeam::None;
    }
    return GameplayTeam::None;
}

GameplayRole ToGameplayRole(PlayerRole role) {
    return role == PlayerRole::Goalie ? GameplayRole::Goalie : GameplayRole::Skater;
}

PlayerSlot ToPlayerSlot(Team team, PlayerRole role, int index) {
    if (team == Team::Home && role == PlayerRole::Goalie) {
        return PlayerSlot::HomeGoalie;
    }
    if (team == Team::Away && role == PlayerRole::Goalie) {
        return PlayerSlot::AwayGoalie;
    }
    if (team == Team::Home && role == PlayerRole::Skater) {
        if (index == 0) {
            return PlayerSlot::HomeSkater0;
        }
        if (index == 1) {
            return PlayerSlot::HomeSkater1;
        }
        if (index == 2) {
            return PlayerSlot::HomeSkater2;
        }
    }
    if (team == Team::Away && role == PlayerRole::Skater) {
        if (index == 0) {
            return PlayerSlot::AwaySkater0;
        }
        if (index == 1) {
            return PlayerSlot::AwaySkater1;
        }
        if (index == 2) {
            return PlayerSlot::AwaySkater2;
        }
    }
    return PlayerSlot::None;
}

uint32_t ToPlayerIndex(Team team, PlayerRole role, int index) {
    if (team == Team::Home) {
        return role == PlayerRole::Goalie ? 3u : static_cast<uint32_t>(index);
    }
    if (team == Team::Away) {
        return role == PlayerRole::Goalie ? 7u : static_cast<uint32_t>(4 + index);
    }
    return 0u;
}

void AddMesh(Entity entity, const char* mesh, const char* material) {
    MeshRendererComponent& renderer = entity.AddComponent<MeshRendererComponent>();
    renderer.meshName = mesh;
    renderer.materialName = material;
}

void Reparent(Scene& scene, Entity child, Entity parent) {
    scene.SetParent(child, parent, /*keepWorldTransform=*/false);
}

// Physics setup helpers (defaults for the gameplay phase to build on; see plan
// section 17). Values are placeholders, not final gameplay tuning.

RigidBodyComponent& AddRigidBody(Entity entity, RigidBodyType type, PhysicsLayer layer, const char* material,
                                 float mass = 1.0f, bool useGravity = true) {
    RigidBodyComponent rb;
    rb.type = type;
    rb.layer = layer;
    rb.materialName = material;
    rb.mass = mass;
    rb.useGravity = useGravity;
    return entity.AddComponent<RigidBodyComponent>(rb);
}

void AddBoxCollider(Entity entity, const glm::vec3& halfExtents, const glm::vec3& offset = glm::vec3(0.0f),
                    bool isTrigger = false) {
    BoxColliderComponent box;
    box.halfExtents = halfExtents;
    box.offset = offset;
    box.isTrigger = isTrigger;
    entity.AddComponent<BoxColliderComponent>(box);
}

void AddCapsuleCollider(Entity entity, float radius, float halfHeight, const glm::vec3& offset = glm::vec3(0.0f)) {
    CapsuleColliderComponent capsule;
    capsule.radius = radius;
    capsule.halfHeight = halfHeight;
    capsule.offset = offset;
    entity.AddComponent<CapsuleColliderComponent>(capsule);
}

// Adds a static board wall (collision only) as a child of the rink.
Entity MakeBoardWall(Scene& scene, Entity rink, const char* name, const glm::vec3& position,
                     const glm::vec3& halfExtents) {
    Entity wall = MakeEntity(scene, name, position);
    AddRigidBody(wall, RigidBodyType::Static, PhysicsLayer::Rink, "Boards");
    AddBoxCollider(wall, halfExtents);
    Reparent(scene, wall, rink);
    return wall;
}

} // namespace

void HockeyRinkTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities(
            "Create Hockey Rink",
            [](Scene& scene) -> std::vector<UUID> {
                Entity rink = MakeEntity(scene, "Rink", {0.0f, 0.0f, 0.0f});
                rink.AddComponent<RinkComponent>();
                rink.AddComponent<PlayAreaComponent>();
                OutOfPlayComponent outOfPlay;
                outOfPlay.resetPosition = {0.0f, 0.1f, 0.0f};
                rink.AddComponent<OutOfPlayComponent>(outOfPlay);

                Entity ice = MakeEntity(scene, "Ice", {0.0f, 0.0f, 0.0f});
                ice.GetComponent<TransformComponent>().localScale = {60.0f, 1.0f, 120.0f};
                AddMesh(ice, "BuiltIn.RinkPlane", "BuiltIn.Ice");
                // Static floor collider (absolute size; physics ignores the
                // entity's visual scale). Top surface sits at y = 0.
                AddRigidBody(ice, RigidBodyType::Static, PhysicsLayer::Rink, "Ice");
                AddBoxCollider(ice, {30.0f, 0.5f, 60.0f}, {0.0f, -0.5f, 0.0f});
                Reparent(scene, ice, rink);

                Entity boards = MakeEntity(scene, "Boards", {0.0f, 1.0f, 0.0f});
                boards.GetComponent<TransformComponent>().localScale = {62.0f, 2.0f, 122.0f};
                AddMesh(boards, "BuiltIn.Cube", "BuiltIn.Boards");
                Reparent(scene, boards, rink);

                // Perimeter board walls (collision only) keep the puck/players
                // inside the 60 x 120 play area, 2 m tall.
                MakeBoardWall(scene, rink, "Board North", {0.0f, 1.0f, 60.0f}, {30.0f, 1.0f, 0.25f});
                MakeBoardWall(scene, rink, "Board South", {0.0f, 1.0f, -60.0f}, {30.0f, 1.0f, 0.25f});
                MakeBoardWall(scene, rink, "Board East", {30.0f, 1.0f, 0.0f}, {0.25f, 1.0f, 60.0f});
                MakeBoardWall(scene, rink, "Board West", {-30.0f, 1.0f, 0.0f}, {0.25f, 1.0f, 60.0f});

                return {rink.GetUUID()};
            }),
        context);
}

void HockeySpawnTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities(
            "Create Hockey Spawns",
            [](Scene& scene) -> std::vector<UUID> {
                Entity root = MakeEntity(scene, "Spawn Points", {0.0f, 0.0f, 0.0f});

                struct SpawnDef {
                    const char* name;
                    Team team;
                    glm::vec3 position;
                };
                const SpawnDef defs[] = {
                    {"Home Spawn 0", Team::Home, {-9.0f, 0.0f, -22.0f}},
                    {"Home Spawn 1", Team::Home, {-3.0f, 0.0f, -22.0f}},
                    {"Home Spawn 2", Team::Home, {3.0f, 0.0f, -22.0f}},
                    {"Home Spawn 3", Team::Home, {9.0f, 0.0f, -22.0f}},
                    {"Away Spawn 0", Team::Away, {-9.0f, 0.0f, 22.0f}},
                    {"Away Spawn 1", Team::Away, {-3.0f, 0.0f, 22.0f}},
                    {"Away Spawn 2", Team::Away, {3.0f, 0.0f, 22.0f}},
                    {"Away Spawn 3", Team::Away, {9.0f, 0.0f, 22.0f}},
                };

                for (const SpawnDef& def : defs) {
                    Entity spawn = MakeEntity(scene, def.name, def.position);
                    SpawnPointComponent marker;
                    marker.team = def.team;
                    marker.faceoffSpawn = false;
                    spawn.AddComponent<SpawnPointComponent>(marker);
                    spawn.AddComponent<TeamComponent>(TeamComponent{def.team});
                    Reparent(scene, spawn, root);
                }
                return {root.GetUUID()};
            }),
        context);
}

void HockeyPlayerTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities(
            "Create Player Bodies",
            [](Scene& scene) -> std::vector<UUID> {
                Entity root = MakeEntity(scene, "Players", {0.0f, 0.0f, 0.0f});

                struct PlayerDef {
                    const char* name;
                    Team team;
                    PlayerRole role;
                    int index;
                    glm::vec3 position; // capsule center; y so feet rest at ice (y=0)
                };
                // Capsule is r=0.4, halfHeight=0.5 -> ~1.8 m tall; center at y=0.9.
                constexpr float kCenterY = 0.9f;
                const PlayerDef defs[] = {
                    {"Home Skater 0", Team::Home, PlayerRole::Skater, 0, {-6.0f, kCenterY, -20.0f}},
                    {"Home Skater 1", Team::Home, PlayerRole::Skater, 1, {0.0f, kCenterY, -20.0f}},
                    {"Home Skater 2", Team::Home, PlayerRole::Skater, 2, {6.0f, kCenterY, -20.0f}},
                    {"Home Goalie", Team::Home, PlayerRole::Goalie, 0, {0.0f, kCenterY, -45.0f}},
                    {"Away Skater 0", Team::Away, PlayerRole::Skater, 0, {-6.0f, kCenterY, 20.0f}},
                    {"Away Skater 1", Team::Away, PlayerRole::Skater, 1, {0.0f, kCenterY, 20.0f}},
                    {"Away Skater 2", Team::Away, PlayerRole::Skater, 2, {6.0f, kCenterY, 20.0f}},
                    {"Away Goalie", Team::Away, PlayerRole::Goalie, 0, {0.0f, kCenterY, 45.0f}},
                };

                for (const PlayerDef& def : defs) {
                    const bool goalie = def.role == PlayerRole::Goalie;
                    Entity player = MakeEntity(scene, def.name, def.position);
                    // Dynamic capsule body on the Player/Goalie layer (placeholder
                    // mass; gameplay tuning happens in a later phase).
                    AddRigidBody(player, RigidBodyType::Dynamic, goalie ? PhysicsLayer::Goalie : PhysicsLayer::Player,
                                 goalie ? "GoalieBody" : "PlayerBody", /*mass=*/80.0f, /*useGravity=*/true);
                    AddCapsuleCollider(player, /*radius=*/0.4f, /*halfHeight=*/0.5f);
                    player.AddComponent<TeamComponent>(TeamComponent{def.team});
                    player.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{def.role});
                    PlayerComponent gameplayPlayer;
                    gameplayPlayer.playerIndex = ToPlayerIndex(def.team, def.role, def.index);
                    gameplayPlayer.slot = ToPlayerSlot(def.team, def.role, def.index);
                    gameplayPlayer.team = ToGameplayTeam(def.team);
                    gameplayPlayer.role = ToGameplayRole(def.role);
                    gameplayPlayer.controlledByLocalInput = gameplayPlayer.playerIndex == 0;
                    gameplayPlayer.controlledByAI = gameplayPlayer.playerIndex != 0;
                    player.AddComponent<PlayerComponent>(gameplayPlayer);
                    if (goalie) {
                        player.AddComponent<GoalieComponent>();
                    } else {
                        player.AddComponent<SkaterComponent>();
                    }
                    player.AddComponent<PlayerRuntimeComponent>();
                    StickComponent stick;
                    stick.ownerPlayer = player.GetUUID();
                    player.AddComponent<StickComponent>(stick);
                    player.AddComponent<ShotComponent>();
                    player.AddComponent<PassComponent>();
                    player.AddComponent<CheckComponent>();
                    // Visible placeholder body so authors can see the capsule.
                    AddMesh(player, "BuiltIn.Cube", "BuiltIn.Boards");
                    player.GetComponent<TransformComponent>().localScale = {0.8f, 1.8f, 0.8f};
                    Reparent(scene, player, root);
                }
                return {root.GetUUID()};
            }),
        context);
}

void HockeyGoalTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(EditorCommands::SpawnEntities(
                                 "Create Hockey Goals",
                                 [](Scene& scene) -> std::vector<UUID> {
                                     const glm::vec3 goalHalfExtents{0.9f, 0.6f, 0.5f};

                                     Entity home = MakeEntity(scene, "Home Goal", {0.0f, 1.0f, -55.0f});
                                     home.AddComponent<GoalComponent>(GoalComponent{Team::Home});
                                     GoalGameplayComponent homeGameplayGoal;
                                     homeGameplayGoal.defendingTeam = GameplayTeam::Home;
                                     homeGameplayGoal.scoringTeam = GameplayTeam::Away;
                                     home.AddComponent<GoalGameplayComponent>(homeGameplayGoal);
                                     AddMesh(home, "BuiltIn.Cube", "BuiltIn.GoalNet");
                                     AddRigidBody(home, RigidBodyType::Static, PhysicsLayer::Goal, "Trigger");
                                     AddBoxCollider(home, goalHalfExtents, glm::vec3(0.0f), /*isTrigger=*/true);
                                     home.AddComponent<TriggerComponent>(TriggerComponent{"Goal", false, false, true});

                                     Entity away = MakeEntity(scene, "Away Goal", {0.0f, 1.0f, 55.0f});
                                     away.AddComponent<GoalComponent>(GoalComponent{Team::Away});
                                     GoalGameplayComponent awayGameplayGoal;
                                     awayGameplayGoal.defendingTeam = GameplayTeam::Away;
                                     awayGameplayGoal.scoringTeam = GameplayTeam::Home;
                                     away.AddComponent<GoalGameplayComponent>(awayGameplayGoal);
                                     AddMesh(away, "BuiltIn.Cube", "BuiltIn.GoalNet");
                                     AddRigidBody(away, RigidBodyType::Static, PhysicsLayer::Goal, "Trigger");
                                     AddBoxCollider(away, goalHalfExtents, glm::vec3(0.0f), /*isTrigger=*/true);
                                     away.AddComponent<TriggerComponent>(TriggerComponent{"Goal", false, false, true});

                                     return {home.GetUUID(), away.GetUUID()};
                                 }),
                             context);
}

void HockeyPuckTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities("Create Puck Spawn",
                                      [](Scene& scene) -> std::vector<UUID> {
                                          Entity puck = MakeEntity(scene, "Puck Spawn", {0.0f, 0.1f, 0.0f});
                                          puck.AddComponent<PuckComponent>();
                                          puck.AddComponent<PuckGameplayComponent>();
                                          puck.AddComponent<PuckRuntimeComponent>();
                                          AddMesh(puck, "BuiltIn.PuckCylinder", "BuiltIn.PuckBlack");
                                          AddRigidBody(puck, RigidBodyType::Dynamic, PhysicsLayer::Puck, "PuckRubber",
                                                       /*mass=*/0.17f, /*useGravity=*/true);
                                          CylinderColliderComponent collider;
                                          collider.radius = 0.1f;
                                          collider.halfHeight = 0.02f;
                                          puck.AddComponent<CylinderColliderComponent>(collider);
                                          return {puck.GetUUID()};
                                      }),
        context);
}

void HockeyFaceoffTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities("Create Faceoff Spots",
                                      [](Scene& scene) -> std::vector<UUID> {
                                          Entity root = MakeEntity(scene, "Faceoff Spawns", {0.0f, 0.0f, 0.0f});

                                          struct FaceoffSpawnDef {
                                              const char* prefix;
                                              Team team;
                                              float z0;
                                              float z1;
                                          };
                                          const FaceoffSpawnDef defs[] = {
                                              {"Neutral Faceoff Spawn", Team::None, -4.0f, 4.0f},
                                              {"Home Penalty Faceoff Spawn", Team::Home, -32.0f, -24.0f},
                                              {"Away Penalty Faceoff Spawn", Team::Away, 24.0f, 32.0f},
                                          };
                                          constexpr float xs[] = {-9.0f, -3.0f, 3.0f, 9.0f};

                                          for (const FaceoffSpawnDef& def : defs) {
                                              int index = 0;
                                              for (const float z : {def.z0, def.z1}) {
                                                  for (const float x : xs) {
                                                      Entity spawn = MakeEntity(
                                                          scene, std::string(def.prefix) + " " + std::to_string(index),
                                                          {x, 0.0f, z});
                                                      SpawnPointComponent marker;
                                                      marker.team = def.team;
                                                      marker.faceoffSpawn = true;
                                                      spawn.AddComponent<SpawnPointComponent>(marker);
                                                      Reparent(scene, spawn, root);
                                                      ++index;
                                                  }
                                              }
                                          }

                                          return {root.GetUUID()};
                                      }),
        context);
}

void HockeyCameraRigTool::OnSelected(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return;
    }
    context.undoRedo.Execute(
        EditorCommands::SpawnEntities("Create Camera Rig",
                                      [](Scene& scene) -> std::vector<UUID> {
                                          Entity rig = MakeEntity(scene, "Gameplay Camera Rig", {0.0f, 20.0f, -75.0f});
                                          rig.GetComponent<TransformComponent>().localRotation = {15.0f, 0.0f, 0.0f};
                                          rig.AddComponent<CameraRigMarkerComponent>();
                                          rig.AddComponent<CameraComponent>(); // not primary
                                          return {rig.GetUUID()};
                                      }),
        context);
}

} // namespace Hockey
