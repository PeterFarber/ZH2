#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

void RunContactEventTests() {
    HockeyTest::BeginSuite("ContactEventTests");

    Physics::Init();

    PhysicsWorld world;
    // Keep bodies awake so the resting contact stays live and a clean
    // contact-removed callback fires when we separate them below.
    PhysicsSettings settings = MakeDefaultPhysicsSettings();
    settings.enableSleeping = false;
    world.Init(settings);

    Scene scene("Contacts");

    Entity floor = scene.CreateEntity("Floor");
    floor.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    {
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Static;
        rb.layer = PhysicsLayer::Static;
        floor.AddComponent<RigidBodyComponent>(rb);
        BoxColliderComponent box;
        box.halfExtents = glm::vec3(5.0f, 0.5f, 5.0f);
        floor.AddComponent<BoxColliderComponent>(box);
    }

    Entity box = scene.CreateEntity("Box");
    box.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 3.0f, 0.0f);
    {
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Dynamic;
        rb.layer = PhysicsLayer::Puck;
        box.AddComponent<RigidBodyComponent>(rb);
        box.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    }

    world.CreateBodiesFromScene(scene);

    const UUID floorId = floor.GetUUID();
    const UUID boxId = box.GetUUID();

    bool gotStart = false;
    bool uuidsMatch = false;
    bool normalOk = false;
    for (int i = 0; i < 180; ++i) {
        world.Step(1.0f / 60.0f);
        for (const PhysicsContactEvent& e : world.DrainContactEvents()) {
            if (e.type == PhysicsContactEvent::Type::Started) {
                gotStart = true;
                if ((e.entityA == boxId && e.entityB == floorId) || (e.entityA == floorId && e.entityB == boxId)) {
                    uuidsMatch = true;
                }
                const float len = glm::length(e.contactNormal);
                if (len > 0.5f && len < 1.5f) {
                    normalOk = true;
                }
            }
        }
    }
    HK_CHECK_MSG(gotStart, "contact started event generated");
    HK_CHECK_MSG(uuidsMatch, "contact includes entity UUIDs");
    HK_CHECK_MSG(normalOk, "contact normal is roughly unit length");

    // Contact ended: lift the box far away so the contact separates.
    world.SetBodyTransform(box, glm::vec3(0.0f, 60.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    bool gotEnded = false;
    for (int i = 0; i < 10; ++i) {
        world.Step(1.0f / 60.0f);
        for (const PhysicsContactEvent& e : world.DrainContactEvents()) {
            if (e.type == PhysicsContactEvent::Type::Ended) {
                gotEnded = true;
            }
        }
    }
    HK_CHECK_MSG(gotEnded, "contact ended event generated");

    // Destroyed entity must not crash the event drain.
    world.DestroyBody(box);
    for (int i = 0; i < 5; ++i) {
        world.Step(1.0f / 60.0f);
        world.DrainContactEvents();
    }
    HK_CHECK_MSG(true, "draining events after entity destruction is safe");

    world.Shutdown();
}
