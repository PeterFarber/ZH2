#include "Test.hpp"

#include <string>

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Gizmos/GizmoOperation.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Editor/Tools/ToolManager.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"

using namespace Hockey;

namespace {

struct ToolFixture {
    Scene scene{"ToolTest"};
    EditorContext context;
    ToolFixture() {
        context.activeScene = &scene;
        RegisterEditorTools(context.toolManager);
    }
};

// Helper: does an entity with the given name and component T exist?
template <typename T> bool HasNamed(Scene& scene, const std::string& name) {
    Entity entity = scene.FindEntityByName(name);
    return entity && entity.HasComponent<T>();
}

} // namespace

void RunToolTests() {
    HockeyTest::BeginSuite("ToolTests");

    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterGameplayComponents();

    // --- transform tools drive the gizmo mode and stay active ----------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;

        ctx.toolManager.Activate("Select", ctx);
        HK_CHECK_MSG(ctx.gizmoOperation == GizmoOperation::None, "Select tool sets gizmo None");
        HK_CHECK_MSG(ctx.toolManager.ActiveTool() != nullptr &&
                         std::string(ctx.toolManager.ActiveTool()->Name()) == "Select",
                     "Select is active");

        ctx.toolManager.Activate("Move", ctx);
        HK_CHECK_MSG(ctx.gizmoOperation == GizmoOperation::Translate, "Move tool sets gizmo Translate");

        ctx.toolManager.Activate("Rotate", ctx);
        HK_CHECK_MSG(ctx.gizmoOperation == GizmoOperation::Rotate, "Rotate tool sets gizmo Rotate");

        ctx.toolManager.Activate("Scale", ctx);
        HK_CHECK_MSG(ctx.gizmoOperation == GizmoOperation::Scale, "Scale tool sets gizmo Scale");
        HK_CHECK_MSG(std::string(ctx.toolManager.ActiveTool()->Name()) == "Scale", "Scale is active");
    }

    // --- instant tools do not replace the active persistent tool -------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        ctx.toolManager.Activate("Move", ctx);
        ctx.toolManager.Activate("Hockey Puck", ctx);
        HK_CHECK_MSG(ctx.toolManager.ActiveTool() != nullptr &&
                         std::string(ctx.toolManager.ActiveTool()->Name()) == "Move",
                     "instant tool leaves Move active");
    }

    // --- HockeyRinkTool ------------------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Rink", ctx);
        HK_CHECK_MSG(HasNamed<RinkComponent>(fix.scene, "Rink"), "rink has RinkComponent");
        HK_CHECK_MSG(HasNamed<PlayAreaComponent>(fix.scene, "Rink"), "rink has PlayAreaComponent");
        HK_CHECK_MSG(HasNamed<OutOfPlayComponent>(fix.scene, "Rink"), "rink has OutOfPlayComponent");
        HK_CHECK_MSG(fix.scene.FindEntityByName("Ice"), "rink creates Ice child");
        HK_CHECK_MSG(fix.scene.FindEntityByName("Boards"), "rink creates Boards child");
        // Rink, Ice, Boards (visual) + 4 board-wall colliders.
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 7);

        ctx.undoRedo.Undo(ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before);
        HK_CHECK_MSG(!fix.scene.FindEntityByName("Rink"), "undo removes rink");

        ctx.undoRedo.Redo(ctx);
        HK_CHECK_MSG(HasNamed<RinkComponent>(fix.scene, "Rink"), "redo restores rink");
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 7);
    }

    // --- HockeySpawnTool: 8 spawns under a root ------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Spawns", ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 9); // root + 8

        Entity homeGoalie = fix.scene.FindEntityByName("Home Goalie Spawn 0");
        HK_CHECK_MSG(homeGoalie && homeGoalie.HasComponent<SpawnPointComponent>(), "spawn has SpawnPointComponent");
        if (homeGoalie) {
            const auto& spawn = homeGoalie.GetComponent<SpawnPointComponent>();
            HK_CHECK_MSG(spawn.team == Team::Home && spawn.role == PlayerRole::Goalie, "home goalie spawn data");
            HK_CHECK_MSG(homeGoalie.HasComponent<TeamComponent>() && homeGoalie.HasComponent<PlayerRoleComponent>(),
                         "spawn has team+role components");
        }

        ctx.undoRedo.Undo(ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before);
    }

    // --- HockeyGoalTool: two goals -------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Goals", ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 2);
        Entity home = fix.scene.FindEntityByName("Home Goal");
        Entity away = fix.scene.FindEntityByName("Away Goal");
        HK_CHECK_MSG(home && home.HasComponent<GoalComponent>(), "home goal has GoalComponent");
        HK_CHECK_MSG(away && away.HasComponent<GoalComponent>(), "away goal has GoalComponent");
        HK_CHECK_MSG(home && home.HasComponent<GoalGameplayComponent>(), "home goal has gameplay goal component");
        HK_CHECK_MSG(away && away.HasComponent<GoalGameplayComponent>(), "away goal has gameplay goal component");
        if (home && away) {
            HK_CHECK_MSG(home.GetComponent<GoalComponent>().defendingTeam == Team::Home, "home goal team");
            HK_CHECK_MSG(away.GetComponent<GoalComponent>().defendingTeam == Team::Away, "away goal team");
        }
        ctx.undoRedo.Undo(ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before);
    }

    // --- HockeyPlayerTool: player bodies are gameplay-ready ------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        ctx.toolManager.Activate("Hockey Players", ctx);
        Entity homeSkater = fix.scene.FindEntityByName("Home Skater 0");
        Entity homeGoalie = fix.scene.FindEntityByName("Home Goalie");
        HK_CHECK_MSG(homeSkater && homeSkater.HasComponent<PlayerComponent>(), "skater has PlayerComponent");
        HK_CHECK_MSG(homeSkater && homeSkater.HasComponent<SkaterComponent>(), "skater has SkaterComponent");
        HK_CHECK_MSG(homeSkater && homeSkater.HasComponent<StickComponent>(), "skater has StickComponent");
        HK_CHECK_MSG(homeSkater && homeSkater.HasComponent<ShotComponent>(), "skater has ShotComponent");
        HK_CHECK_MSG(homeGoalie && homeGoalie.HasComponent<PlayerComponent>(), "goalie has PlayerComponent");
        HK_CHECK_MSG(homeGoalie && homeGoalie.HasComponent<GoalieComponent>(), "goalie has GoalieComponent");
        if (homeSkater) {
            const PlayerComponent& player = homeSkater.GetComponent<PlayerComponent>();
            HK_CHECK_EQ(player.playerIndex, 0u);
            HK_CHECK_EQ(player.slot, PlayerSlot::HomeSkater0);
            HK_CHECK_MSG(player.controlledByLocalInput, "home skater 0 receives local input");
        }
        if (homeSkater && homeSkater.HasComponent<RigidBodyComponent>() &&
            homeSkater.HasComponent<CapsuleColliderComponent>()) {
            const RigidBodyComponent& body = homeSkater.GetComponent<RigidBodyComponent>();
            const CapsuleColliderComponent& capsule = homeSkater.GetComponent<CapsuleColliderComponent>();
            HK_CHECK_MSG(body.type == RigidBodyType::Dynamic, "skater body is dynamic");
            HK_CHECK_MSG(body.layer == PhysicsLayer::Player, "skater body uses Player collision layer");
            HK_CHECK_EQ(body.materialName, std::string("PlayerBody"));
            HK_CHECK_MSG(!capsule.isTrigger, "skater capsule is solid");
            HK_CHECK_MSG(capsule.radius > 0.0f && capsule.halfHeight > 0.0f, "skater capsule dimensions are valid");
        } else {
            HK_CHECK_MSG(false, "skater has role-specific solid capsule body");
        }
        if (homeGoalie && homeGoalie.HasComponent<RigidBodyComponent>() &&
            homeGoalie.HasComponent<CapsuleColliderComponent>()) {
            const RigidBodyComponent& body = homeGoalie.GetComponent<RigidBodyComponent>();
            const CapsuleColliderComponent& capsule = homeGoalie.GetComponent<CapsuleColliderComponent>();
            HK_CHECK_MSG(body.type == RigidBodyType::Dynamic, "goalie body is dynamic");
            HK_CHECK_MSG(body.layer == PhysicsLayer::Goalie, "goalie body uses Goalie collision layer");
            HK_CHECK_EQ(body.materialName, std::string("GoalieBody"));
            HK_CHECK_MSG(!capsule.isTrigger, "goalie capsule is solid");
            HK_CHECK_MSG(capsule.radius > 0.0f && capsule.halfHeight > 0.0f, "goalie capsule dimensions are valid");
        } else {
            HK_CHECK_MSG(false, "goalie has role-specific solid capsule body");
        }
    }

    // --- HockeyPuckTool ------------------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        ctx.toolManager.Activate("Hockey Puck", ctx);
        HK_CHECK_MSG(HasNamed<PuckComponent>(fix.scene, "Puck Spawn"), "puck has PuckComponent");
        HK_CHECK_MSG(HasNamed<PuckGameplayComponent>(fix.scene, "Puck Spawn"), "puck has PuckGameplayComponent");
        HK_CHECK_MSG(HasNamed<PuckRuntimeComponent>(fix.scene, "Puck Spawn"), "puck has PuckRuntimeComponent");
    }

    // --- HockeyFaceoffTool: three spots --------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Faceoff Spots", ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 3);
        HK_CHECK_MSG(HasNamed<FaceoffSpotComponent>(fix.scene, "Center Faceoff Spot"), "center faceoff spot");
        HK_CHECK_MSG(HasNamed<FaceoffGameplayComponent>(fix.scene, "Center Faceoff Spot"), "center gameplay faceoff");
        HK_CHECK_MSG(HasNamed<FaceoffSpotComponent>(fix.scene, "Home Defensive Faceoff Spot"), "home faceoff spot");
        HK_CHECK_MSG(HasNamed<FaceoffSpotComponent>(fix.scene, "Away Defensive Faceoff Spot"), "away faceoff spot");
    }

    // --- HockeyCameraRigTool -------------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        ctx.toolManager.Activate("Hockey Camera Rig", ctx);
        HK_CHECK_MSG(HasNamed<CameraRigMarkerComponent>(fix.scene, "Gameplay Camera Rig"), "rig has marker");
        HK_CHECK_MSG(HasNamed<CameraComponent>(fix.scene, "Gameplay Camera Rig"), "rig has CameraComponent");
    }

    // --- Light tools ---------------------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;

        ctx.toolManager.Activate("Directional Light", ctx);
        Entity dir = fix.scene.FindEntityByName("Directional Light");
        HK_CHECK_MSG(dir && dir.HasComponent<LightComponent>(), "directional light created");
        if (dir) {
            HK_CHECK_MSG(dir.GetComponent<LightComponent>().type == LightComponent::Type::Directional,
                         "directional light type");
        }

        ctx.toolManager.Activate("Point Light", ctx);
        Entity point = fix.scene.FindEntityByName("Point Light");
        HK_CHECK_MSG(point && point.GetComponent<LightComponent>().type == LightComponent::Type::Point,
                     "point light type");

        ctx.toolManager.Activate("Spot Light", ctx);
        Entity spot = fix.scene.FindEntityByName("Spot Light");
        HK_CHECK_MSG(spot && spot.GetComponent<LightComponent>().type == LightComponent::Type::Spot, "spot light type");
    }

    // --- spawning marks the scene dirty --------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        HK_CHECK_MSG(!ctx.sceneDirty, "scene starts clean");
        ctx.toolManager.Activate("Hockey Puck", ctx);
        HK_CHECK_MSG(ctx.sceneDirty, "spawning marks dirty");
    }
}
