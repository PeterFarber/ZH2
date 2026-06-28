#include "Test.hpp"

#include <string>

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Paths.hpp"
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
    AssetManager assets;
    EditorContext context;
    ToolFixture() {
        HK_CHECK_MSG(static_cast<bool>(assets.Init(AssetManager::DefaultCreateInfo(Paths::Get().root))),
                     "tool test asset manager initializes");
        context.activeScene = &scene;
        context.assetManager = &assets;
        RegisterEditorTools(context.toolManager);
    }

    ~ToolFixture() {
        assets.Shutdown();
    }
};

// Helper: does an entity with the given name and component T exist?
template <typename T> bool HasNamed(Scene& scene, const std::string& name) {
    Entity entity = scene.FindEntityByName(name);
    return entity && entity.HasComponent<T>();
}

void CheckVisualAssetRefs(Entity entity, const char* label) {
    HK_CHECK_MSG(entity && entity.HasComponent<MeshRendererComponent>(), std::string(label) + " has mesh renderer");
    if (entity && entity.HasComponent<MeshRendererComponent>()) {
        const MeshRendererComponent& renderer = entity.GetComponent<MeshRendererComponent>();
        HK_CHECK_MSG(renderer.meshAsset != 0, std::string(label) + " uses mesh asset");
        HK_CHECK_MSG(renderer.materialAsset != 0, std::string(label) + " uses material asset");
    }
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
        CheckVisualAssetRefs(fix.scene.FindEntityByName("Ice"), "ice");
        CheckVisualAssetRefs(fix.scene.FindEntityByName("Boards"), "boards");
        // Rink, Ice, Boards (visual) + 4 board-wall colliders.
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 7);

        ctx.undoRedo.Undo(ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before);
        HK_CHECK_MSG(!fix.scene.FindEntityByName("Rink"), "undo removes rink");

        ctx.undoRedo.Redo(ctx);
        HK_CHECK_MSG(HasNamed<RinkComponent>(fix.scene, "Rink"), "redo restores rink");
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 7);
    }

    // --- HockeySpawnTool: 8 normal spawn points under a root -----------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Spawns", ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 9); // root + 8

        Entity homeSpawn = fix.scene.FindEntityByName("Home Spawn 0");
        HK_CHECK_MSG(homeSpawn && homeSpawn.HasComponent<SpawnPointComponent>(), "spawn has SpawnPointComponent");
        if (homeSpawn) {
            const auto& spawn = homeSpawn.GetComponent<SpawnPointComponent>();
            HK_CHECK_MSG(spawn.team == Team::Home, "home spawn data");
            HK_CHECK_MSG(!spawn.faceoffSpawn, "normal spawn is not faceoff spawn");
            HK_CHECK_MSG(homeSpawn.HasComponent<TeamComponent>(), "spawn has team component");
            HK_CHECK_MSG(homeSpawn.HasComponent<PlayerRoleComponent>(), "spawn exposes editable role");
            HK_CHECK_MSG(homeSpawn.GetComponent<PlayerRoleComponent>().role == PlayerRole::Skater,
                         "home spawn 0 defaults to skater role");
            Entity homeGoalieSpawn = fix.scene.FindEntityByName("Home Spawn 3");
            HK_CHECK_MSG(homeGoalieSpawn && homeGoalieSpawn.HasComponent<PlayerRoleComponent>(),
                         "home spawn 3 exposes editable goalie role");
            if (homeGoalieSpawn && homeGoalieSpawn.HasComponent<PlayerRoleComponent>()) {
                HK_CHECK_MSG(homeGoalieSpawn.GetComponent<PlayerRoleComponent>().role == PlayerRole::Goalie,
                             "home spawn 3 defaults to goalie role");
            }
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
        CheckVisualAssetRefs(home, "home goal");
        CheckVisualAssetRefs(away, "away goal");
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
            CheckVisualAssetRefs(homeSkater, "home skater");
        }
        if (homeGoalie) {
            CheckVisualAssetRefs(homeGoalie, "home goalie");
        }
        if (homeSkater && homeSkater.HasComponent<RigidBodyComponent>() &&
            homeSkater.HasComponent<CapsuleColliderComponent>()) {
            const RigidBodyComponent& body = homeSkater.GetComponent<RigidBodyComponent>();
            const CapsuleColliderComponent& capsule = homeSkater.GetComponent<CapsuleColliderComponent>();
            HK_CHECK_MSG(body.type == RigidBodyType::Dynamic, "skater body is a dynamic physics actor");
            HK_CHECK_MSG(body.collisionDetection == CollisionDetectionMode::Continuous,
                         "skater body uses continuous collision detection");
            HK_CHECK_MSG(!body.useGravity, "skater body ignores gravity");
            HK_CHECK_MSG(!body.allowSleeping, "skater body stays awake for gameplay velocity control");
            HK_CHECK_MSG(body.lockTranslationY, "skater body locks vertical translation");
            HK_CHECK_MSG(body.lockRotationX, "skater body locks X rotation");
            HK_CHECK_MSG(!body.lockRotationY, "skater body leaves Y rotation free for gameplay facing");
            HK_CHECK_MSG(body.lockRotationZ, "skater body locks Z rotation");
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
            HK_CHECK_MSG(body.type == RigidBodyType::Dynamic, "goalie body is a dynamic physics actor");
            HK_CHECK_MSG(body.collisionDetection == CollisionDetectionMode::Continuous,
                         "goalie body uses continuous collision detection");
            HK_CHECK_MSG(!body.useGravity, "goalie body ignores gravity");
            HK_CHECK_MSG(!body.allowSleeping, "goalie body stays awake for gameplay velocity control");
            HK_CHECK_MSG(body.lockTranslationY, "goalie body locks vertical translation");
            HK_CHECK_MSG(body.lockRotationX, "goalie body locks X rotation");
            HK_CHECK_MSG(!body.lockRotationY, "goalie body leaves Y rotation free for gameplay facing");
            HK_CHECK_MSG(body.lockRotationZ, "goalie body locks Z rotation");
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
        Entity puck = fix.scene.FindEntityByName("Puck Spawn");
        HK_CHECK_MSG(puck && puck.HasComponent<RigidBodyComponent>(), "puck has RigidBodyComponent");
        CheckVisualAssetRefs(puck, "puck");
        if (puck && puck.HasComponent<RigidBodyComponent>()) {
            HK_CHECK_MSG(puck.GetComponent<RigidBodyComponent>().collisionDetection == CollisionDetectionMode::Continuous,
                         "puck tool authors continuous collision detection");
        }
        if (puck) {
            const glm::vec3 scale = puck.GetComponent<TransformComponent>().localScale;
            HK_CHECK_NEAR(scale.x, 0.2f, 0.0001f);
            HK_CHECK_NEAR(scale.y, 0.04f, 0.0001f);
            HK_CHECK_NEAR(scale.z, 0.2f, 0.0001f);
        }
    }

    // --- HockeyFaceoffTool: 24 faceoff spawn points under a root -------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        const std::size_t before = ctx.activeScene->EntityCount();
        ctx.toolManager.Activate("Hockey Faceoff Spots", ctx);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 25); // root + 8 neutral + 8 home penalty + 8 away penalty

        Entity neutral = fix.scene.FindEntityByName("Neutral Faceoff Spawn 0");
        Entity neutralHomeGoalie = fix.scene.FindEntityByName("Neutral Faceoff Spawn 3");
        Entity neutralAwaySkater = fix.scene.FindEntityByName("Neutral Faceoff Spawn 4");
        Entity neutralAwayGoalie = fix.scene.FindEntityByName("Neutral Faceoff Spawn 7");
        Entity home = fix.scene.FindEntityByName("Home Penalty Faceoff Spawn 0");
        Entity away = fix.scene.FindEntityByName("Away Penalty Faceoff Spawn 0");

        HK_CHECK_MSG(neutral && neutral.HasComponent<SpawnPointComponent>(), "neutral faceoff spawn");
        HK_CHECK_MSG(home && home.HasComponent<SpawnPointComponent>(), "home penalty faceoff spawn");
        HK_CHECK_MSG(away && away.HasComponent<SpawnPointComponent>(), "away penalty faceoff spawn");
        if (neutral && home && away) {
            HK_CHECK(neutral.GetComponent<SpawnPointComponent>().faceoffSpawn);
            HK_CHECK(home.GetComponent<SpawnPointComponent>().faceoffSpawn);
            HK_CHECK(away.GetComponent<SpawnPointComponent>().faceoffSpawn);
            HK_CHECK(neutral.GetComponent<SpawnPointComponent>().team == Team::None);
            HK_CHECK(home.GetComponent<SpawnPointComponent>().team == Team::Home);
            HK_CHECK(away.GetComponent<SpawnPointComponent>().team == Team::Away);
            HK_CHECK_MSG(neutral.HasComponent<TeamComponent>(), "neutral faceoff spawn exposes editable team");
            HK_CHECK_MSG(neutral.GetComponent<TeamComponent>().team == Team::Home,
                         "neutral faceoff spawn uses TeamComponent for player team");
            HK_CHECK_MSG(neutral.HasComponent<PlayerRoleComponent>(), "neutral faceoff spawn exposes editable role");
            HK_CHECK_MSG(neutral.GetComponent<PlayerRoleComponent>().role == PlayerRole::Skater,
                         "neutral faceoff spawn defaults to skater role");
            HK_CHECK_MSG(neutralHomeGoalie && neutralHomeGoalie.GetComponent<SpawnPointComponent>().team == Team::None,
                         "neutral goalie faceoff spawn stays in neutral pool");
            HK_CHECK_MSG(neutralHomeGoalie && neutralHomeGoalie.GetComponent<TeamComponent>().team == Team::Home,
                         "neutral goalie faceoff spawn can target home players");
            HK_CHECK_MSG(neutralHomeGoalie &&
                             neutralHomeGoalie.GetComponent<PlayerRoleComponent>().role == PlayerRole::Goalie,
                         "neutral faceoff spawn 3 defaults to home goalie");
            HK_CHECK_MSG(neutralAwaySkater && neutralAwaySkater.GetComponent<SpawnPointComponent>().team == Team::None,
                         "neutral away faceoff spawn stays in neutral pool");
            HK_CHECK_MSG(neutralAwaySkater && neutralAwaySkater.GetComponent<TeamComponent>().team == Team::Away,
                         "neutral faceoff spawn 4 can target away players");
            HK_CHECK_MSG(neutralAwayGoalie &&
                             neutralAwayGoalie.GetComponent<PlayerRoleComponent>().role == PlayerRole::Goalie,
                         "neutral faceoff spawn 7 defaults to away goalie");
        }
        HK_CHECK_MSG(!fix.scene.FindEntityByName("Center Faceoff Spot"), "old faceoff spot is not authored");
    }

    // --- HockeyCameraRigTool -------------------------------------------------
    {
        ToolFixture fix;
        EditorContext& ctx = fix.context;
        ctx.toolManager.Activate("Hockey Camera Rig", ctx);
        HK_CHECK_MSG(HasNamed<CameraRigMarkerComponent>(fix.scene, "Gameplay Camera Rig"), "rig has marker");
        HK_CHECK_MSG(HasNamed<CameraComponent>(fix.scene, "Gameplay Camera Rig"), "rig has CameraComponent");
        Entity rig = fix.scene.FindEntityByName("Gameplay Camera Rig");
        if (rig && rig.HasComponent<CameraComponent>()) {
            const auto& camera = rig.GetComponent<CameraComponent>();
            HK_CHECK_MSG(camera.followPlayer, "rig follows player by default");
            HK_CHECK_NEAR(camera.followOffset.y, 7.5f, 0.0001f);
            HK_CHECK_NEAR(camera.followOffset.z, 10.0f, 0.0001f);
            HK_CHECK_NEAR(camera.followRotation.x, -30.0f, 0.0001f);
        }
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
