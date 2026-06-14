#include "Test.hpp"

#include <vector>

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Tools/EditorTools.hpp"
#include "Hockey/Editor/Tools/ToolManager.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

struct ToolFixture {
    Scene scene{"PhysicsSetup"};
    EditorContext context;
    ToolFixture() {
        context.activeScene = &scene;
        RegisterEditorTools(context.toolManager);
    }
};

bool HasPhysicsError(const std::vector<SceneValidationIssue>& issues, const char* substr) {
    const std::string needle = substr;
    for (const SceneValidationIssue& issue : issues) {
        if (issue.message.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunHockeyPhysicsSetupTests() {
    HockeyTest::BeginSuite("HockeyPhysicsSetupTests");

    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterPhysicsComponents();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();
    Physics::Init();

    // --- Puck tool produces a dynamic body with collider + material ----------
    {
        ToolFixture fix;
        fix.context.toolManager.Activate("Hockey Puck", fix.context);
        Entity puck = fix.scene.FindEntityByName("Puck Spawn");
        HK_CHECK_MSG(puck && puck.HasComponent<RigidBodyComponent>(), "puck has a rigid body");
        HK_CHECK_MSG(puck && puck.HasComponent<CylinderColliderComponent>(), "puck has a collider");
        if (puck && puck.HasComponent<RigidBodyComponent>()) {
            const RigidBodyComponent& rb = puck.GetComponent<RigidBodyComponent>();
            HK_CHECK_MSG(rb.type == RigidBodyType::Dynamic, "puck is dynamic");
            HK_CHECK_MSG(rb.layer == PhysicsLayer::Puck, "puck uses the Puck layer");
            HK_CHECK_MSG(rb.mass > 0.0f, "puck has positive mass");
            HK_CHECK_MSG(PhysicsMaterialRegistry::Get().Contains(rb.materialName), "puck material is registered");
        }
    }

    // --- Goal tool produces trigger volumes ----------------------------------
    {
        ToolFixture fix;
        fix.context.toolManager.Activate("Hockey Goals", fix.context);
        Entity home = fix.scene.FindEntityByName("Home Goal");
        HK_CHECK_MSG(home && home.HasComponent<RigidBodyComponent>(), "goal has a rigid body");
        HK_CHECK_MSG(home && home.HasComponent<BoxColliderComponent>(), "goal has a box collider");
        HK_CHECK_MSG(home && home.HasComponent<TriggerComponent>(), "goal has a TriggerComponent");
        if (home && home.HasComponent<BoxColliderComponent>()) {
            HK_CHECK_MSG(home.GetComponent<BoxColliderComponent>().isTrigger, "goal collider is a trigger");
        }
        if (home && home.HasComponent<RigidBodyComponent>()) {
            HK_CHECK_MSG(home.GetComponent<RigidBodyComponent>().layer == PhysicsLayer::Goal, "goal uses Goal layer");
        }
    }

    // --- Rink tool produces static ice + board colliders ---------------------
    {
        ToolFixture fix;
        fix.context.toolManager.Activate("Hockey Rink", fix.context);

        Entity ice = fix.scene.FindEntityByName("Ice");
        HK_CHECK_MSG(ice && ice.HasComponent<RigidBodyComponent>(), "ice has a rigid body");
        HK_CHECK_MSG(ice && ice.HasComponent<BoxColliderComponent>(), "ice has a box collider");
        if (ice && ice.HasComponent<RigidBodyComponent>()) {
            HK_CHECK_MSG(ice.GetComponent<RigidBodyComponent>().type == RigidBodyType::Static, "ice is static");
        }

        Entity wall = fix.scene.FindEntityByName("Board North");
        HK_CHECK_MSG(wall && wall.HasComponent<RigidBodyComponent>(), "board wall has a rigid body");
        HK_CHECK_MSG(wall && wall.HasComponent<BoxColliderComponent>(), "board wall has a collider");
    }

    // --- A full puck+rink setup builds bodies and falls onto the ice ---------
    {
        ToolFixture fix;
        fix.context.toolManager.Activate("Hockey Rink", fix.context);
        fix.context.toolManager.Activate("Hockey Puck", fix.context);

        // Drop the puck a little so it falls onto the ice surface (y = 0).
        Entity puck = fix.scene.FindEntityByName("Puck Spawn");
        HK_CHECK_MSG(static_cast<bool>(puck), "puck spawned");
        if (puck) {
            puck.GetComponent<TransformComponent>().localPosition = {0.0f, 3.0f, 0.0f};
        }

        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        world.CreateBodiesFromScene(fix.scene);
        HK_CHECK_MSG(world.BodyCount() >= 6, "rink + puck create multiple bodies");

        for (int i = 0; i < 180; ++i) {
            world.Step(1.0f / 60.0f);
        }
        world.SyncPhysicsToScene(fix.scene);

        if (puck) {
            const float y = puck.GetComponent<TransformComponent>().localPosition.y;
            HK_CHECK_MSG(y < 3.0f, "puck fell under gravity");
            HK_CHECK_MSG(y > -1.0f, "puck came to rest on the ice (did not fall through)");
        }
        world.Shutdown();
    }

    // --- Validation passes for the recommended hockey setup ------------------
    {
        ToolFixture fix;
        fix.context.toolManager.Activate("Hockey Rink", fix.context);
        fix.context.toolManager.Activate("Hockey Goals", fix.context);
        fix.context.toolManager.Activate("Hockey Puck", fix.context);

        const std::vector<SceneValidationIssue> issues = SceneValidator::Validate(fix.scene);
        HK_CHECK_MSG(!HasPhysicsError(issues, "negative"), "no negative-mass errors");
        HK_CHECK_MSG(!HasPhysicsError(issues, "zero/negative"), "no zero-size collider errors");
        HK_CHECK_MSG(!HasPhysicsError(issues, "missing a trigger collider"), "goal trigger setup is complete");
        HK_CHECK_MSG(!HasPhysicsError(issues, "Puck entity is missing"), "puck setup is complete");
    }

    Physics::Shutdown();
}
