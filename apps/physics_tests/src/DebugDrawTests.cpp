#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsDebugDraw.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

void RunDebugDrawTests() {
    HockeyTest::BeginSuite("DebugDrawTests");

    // Pure wireframe builders (no world needed).
    {
        PhysicsDebugDrawList list;
        AppendWireBox(list, glm::vec3(0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                      PhysicsDebugColors::kCollider);
        HK_CHECK_EQ(list.lines.size(), static_cast<std::size_t>(12));

        list.Clear();
        HK_CHECK_MSG(list.lines.empty(), "draw list clears");

        AppendWireSphere(list, glm::vec3(0.0f), 0.5f, PhysicsDebugColors::kCollider, 12);
        HK_CHECK_MSG(list.lines.size() == 36, "sphere is three 12-segment rings");
    }

    Physics::Init();

    PhysicsWorld world;
    world.Init(MakeDefaultPhysicsSettings());

    Scene scene("Debug");

    Entity box = scene.CreateEntity("Box");
    box.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 2.0f, 0.0f);
    {
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Dynamic;
        rb.layer = PhysicsLayer::Puck;
        box.AddComponent<RigidBodyComponent>(rb);
        box.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    }

    Entity trig = scene.CreateEntity("Trigger");
    trig.GetComponent<TransformComponent>().localPosition = glm::vec3(5.0f, 0.0f, 0.0f);
    {
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Static;
        rb.layer = PhysicsLayer::Goal;
        trig.AddComponent<RigidBodyComponent>(rb);
        BoxColliderComponent tbox;
        tbox.isTrigger = true;
        trig.AddComponent<BoxColliderComponent>(tbox);
    }

    world.CreateBodiesFromScene(scene);

    PhysicsDebugDrawList list;
    world.CollectDebugDraw(list);
    HK_CHECK_MSG(!list.lines.empty(), "debug draw produces geometry for bodies");

    bool hasTriggerColor = false;
    bool hasColliderColor = false;
    for (const PhysicsDebugLine& line : list.lines) {
        if (line.color == PhysicsDebugColors::kTrigger) {
            hasTriggerColor = true;
        }
        if (line.color == PhysicsDebugColors::kCollider) {
            hasColliderColor = true;
        }
    }
    HK_CHECK_MSG(hasTriggerColor, "trigger volumes use the trigger colour");
    HK_CHECK_MSG(hasColliderColor, "solid colliders use the collider colour");

    // No bodies -> no geometry after destruction.
    world.DestroyBodies();
    PhysicsDebugDrawList empty;
    world.CollectDebugDraw(empty);
    HK_CHECK_MSG(empty.lines.empty(), "no debug geometry once bodies are destroyed");

    world.Shutdown();
}
