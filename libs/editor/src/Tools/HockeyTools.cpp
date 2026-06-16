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
                    PlayerRole role;
                    int index;
                    glm::vec3 position;
                };
                const SpawnDef defs[] = {
                    {"Home Skater Spawn 0", Team::Home, PlayerRole::Skater, 0, {-6.0f, 0.0f, -20.0f}},
                    {"Home Skater Spawn 1", Team::Home, PlayerRole::Skater, 1, {0.0f, 0.0f, -20.0f}},
                    {"Home Skater Spawn 2", Team::Home, PlayerRole::Skater, 2, {6.0f, 0.0f, -20.0f}},
                    {"Home Goalie Spawn 0", Team::Home, PlayerRole::Goalie, 0, {0.0f, 0.0f, -45.0f}},
                    {"Away Skater Spawn 0", Team::Away, PlayerRole::Skater, 0, {-6.0f, 0.0f, 20.0f}},
                    {"Away Skater Spawn 1", Team::Away, PlayerRole::Skater, 1, {0.0f, 0.0f, 20.0f}},
                    {"Away Skater Spawn 2", Team::Away, PlayerRole::Skater, 2, {6.0f, 0.0f, 20.0f}},
                    {"Away Goalie Spawn 0", Team::Away, PlayerRole::Goalie, 0, {0.0f, 0.0f, 45.0f}},
                };

                for (const SpawnDef& def : defs) {
                    Entity spawn = MakeEntity(scene, def.name, def.position);
                    spawn.AddComponent<SpawnPointComponent>(SpawnPointComponent{def.team, def.role, def.index});
                    spawn.AddComponent<TeamComponent>(TeamComponent{def.team});
                    spawn.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{def.role});
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
                                     AddMesh(home, "BuiltIn.Cube", "BuiltIn.GoalNet");
                                     // Goal scoring volume: a trigger box + TriggerComponent. Gameplay
                                     // (a later phase) decides whether an overlap is a goal.
                                     AddRigidBody(home, RigidBodyType::Static, PhysicsLayer::Goal, "Trigger");
                                     AddBoxCollider(home, goalHalfExtents, glm::vec3(0.0f), /*isTrigger=*/true);
                                     home.AddComponent<TriggerComponent>(TriggerComponent{"Goal", false, false, true});

                                     Entity away = MakeEntity(scene, "Away Goal", {0.0f, 1.0f, 55.0f});
                                     away.AddComponent<GoalComponent>(GoalComponent{Team::Away});
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
                                          AddMesh(puck, "BuiltIn.PuckCylinder", "BuiltIn.PuckBlack");
                                          // Dynamic puck body (placeholder mass/size; tuned in gameplay).
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
                                          Entity center = MakeEntity(scene, "Center Faceoff Spot", {0.0f, 0.0f, 0.0f});
                                          center.AddComponent<FaceoffSpotComponent>(FaceoffSpotComponent{0});

                                          Entity home =
                                              MakeEntity(scene, "Home Defensive Faceoff Spot", {0.0f, 0.0f, -25.0f});
                                          home.AddComponent<FaceoffSpotComponent>(FaceoffSpotComponent{1});

                                          Entity away =
                                              MakeEntity(scene, "Away Defensive Faceoff Spot", {0.0f, 0.0f, 25.0f});
                                          away.AddComponent<FaceoffSpotComponent>(FaceoffSpotComponent{2});

                                          return {center.GetUUID(), home.GetUUID(), away.GetUUID()};
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
