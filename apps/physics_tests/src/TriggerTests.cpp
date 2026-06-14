#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsLayer.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

// Builds a static box trigger volume (centred at origin) plus a dynamic puck
// dropped from above so it falls straight through the trigger.
void BuildTriggerScene(Scene& scene, Entity& outTrigger, Entity& outPuck) {
    outTrigger = scene.CreateEntity("Goal");
    outTrigger.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    RigidBodyComponent trb;
    trb.type = RigidBodyType::Static;
    trb.layer = PhysicsLayer::Goal;
    outTrigger.AddComponent<RigidBodyComponent>(trb);
    BoxColliderComponent tbox;
    tbox.halfExtents = glm::vec3(2.0f, 2.0f, 2.0f);
    tbox.isTrigger = true;
    outTrigger.AddComponent<BoxColliderComponent>(tbox);

    outPuck = scene.CreateEntity("Puck");
    outPuck.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 6.0f, 0.0f);
    RigidBodyComponent prb;
    prb.type = RigidBodyType::Dynamic;
    prb.layer = PhysicsLayer::Puck;
    outPuck.AddComponent<RigidBodyComponent>(prb);
    SphereColliderComponent psphere;
    psphere.radius = 0.2f;
    outPuck.AddComponent<SphereColliderComponent>(psphere);
}

} // namespace

void RunTriggerTests() {
    HockeyTest::BeginSuite("TriggerTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    // Trigger enter/exit + detects puck.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Triggers");
        Entity trigger;
        Entity puck;
        BuildTriggerScene(scene, trigger, puck);
        world.CreateBodiesFromScene(scene);

        const UUID triggerId = trigger.GetUUID();
        const UUID puckId = puck.GetUUID();

        bool entered = false;
        bool exited = false;
        bool detectsPuck = false;
        for (int i = 0; i < 240; ++i) {
            world.Step(1.0f / 60.0f);
            for (const PhysicsTriggerEvent& e : world.DrainTriggerEvents()) {
                if (e.type == PhysicsTriggerEvent::Type::Entered) {
                    entered = true;
                    if (e.triggerEntity == triggerId && e.otherEntity == puckId) {
                        detectsPuck = true;
                    }
                } else if (e.type == PhysicsTriggerEvent::Type::Exited) {
                    exited = true;
                }
            }
        }
        HK_CHECK_MSG(entered, "trigger enter event generated");
        HK_CHECK_MSG(exited, "trigger exit event generated");
        HK_CHECK_MSG(detectsPuck, "trigger detects puck");
        world.Shutdown();
    }

    // Trigger ignores a disabled layer pair.
    {
        CollisionFilter::SetShouldCollide(PhysicsLayer::Goal, PhysicsLayer::Puck, false);

        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("TriggersDisabled");
        Entity trigger;
        Entity puck;
        BuildTriggerScene(scene, trigger, puck);
        world.CreateBodiesFromScene(scene);

        bool anyEvent = false;
        for (int i = 0; i < 240; ++i) {
            world.Step(1.0f / 60.0f);
            if (!world.DrainTriggerEvents().empty()) {
                anyEvent = true;
            }
        }
        HK_CHECK_MSG(!anyEvent, "trigger ignores disabled layer pair");
        world.Shutdown();

        CollisionFilter::ResetDefaults();
    }
}
