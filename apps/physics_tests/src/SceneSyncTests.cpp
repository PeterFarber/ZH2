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

namespace {

Entity MakeEntity(Scene& scene, const char* name, RigidBodyType type, const glm::vec3& pos, bool gravity,
                  bool withCollider) {
    Entity e = scene.CreateEntity(name);
    e.GetComponent<TransformComponent>().localPosition = pos;

    RigidBodyComponent rb;
    rb.type = type;
    rb.useGravity = gravity;
    rb.layer = type == RigidBodyType::Static ? PhysicsLayer::Static : PhysicsLayer::Puck;
    e.AddComponent<RigidBodyComponent>(rb);

    if (withCollider) {
        e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    }
    return e;
}

} // namespace

void RunSceneSyncTests() {
    HockeyTest::BeginSuite("SceneSyncTests");

    Physics::Init();

    // Scene transform creates physics body transform.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Sync");
        Entity e = MakeEntity(scene, "Placed", RigidBodyType::Dynamic, glm::vec3(2.0f, 3.0f, 4.0f), false, true);
        world.CreateBodiesFromScene(scene);
        glm::vec3 pos;
        HK_CHECK_MSG(world.GetBodyPosition(e, pos), "body created from scene");
        HK_CHECK_NEAR(pos.x, 2.0f, 1e-3);
        HK_CHECK_NEAR(pos.y, 3.0f, 1e-3);
        HK_CHECK_NEAR(pos.z, 4.0f, 1e-3);
        world.Shutdown();
    }

    // Dynamic physics body writes back to scene transform.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Sync");
        Entity e = MakeEntity(scene, "Faller", RigidBodyType::Dynamic, glm::vec3(0.0f, 10.0f, 0.0f), true, true);
        world.CreateBodiesFromScene(scene);
        for (int i = 0; i < 60; ++i) {
            world.Step(1.0f / 60.0f);
        }
        world.SyncPhysicsToScene(scene);
        const float syncedY = e.GetComponent<TransformComponent>().localPosition.y;
        HK_CHECK_MSG(syncedY < 9.9f, "dynamic body writes new transform back to scene");
        world.Shutdown();
    }

    // Kinematic scene transform writes to physics.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Sync");
        Entity e = MakeEntity(scene, "Kinematic", RigidBodyType::Kinematic, glm::vec3(0.0f, 0.0f, 0.0f), false, true);
        world.CreateBodiesFromScene(scene);
        e.GetComponent<TransformComponent>().localPosition = glm::vec3(5.0f, 5.0f, 5.0f);
        world.SyncSceneToPhysics(scene);
        glm::vec3 pos;
        world.GetBodyPosition(e, pos);
        HK_CHECK_NEAR(pos.x, 5.0f, 1e-2);
        HK_CHECK_NEAR(pos.y, 5.0f, 1e-2);
        HK_CHECK_NEAR(pos.z, 5.0f, 1e-2);
        world.Shutdown();
    }

    // Destroyed entity removes body on sync.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Sync");
        Entity e = MakeEntity(scene, "Doomed", RigidBodyType::Dynamic, glm::vec3(0.0f, 1.0f, 0.0f), false, true);
        world.CreateBodiesFromScene(scene);
        HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(1));
        scene.DestroyEntity(e);
        world.SyncSceneToPhysics(scene);
        HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(0));
        world.Shutdown();
    }

    // Added collider creates body on sync.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("Sync");
        Entity e = MakeEntity(scene, "LateCollider", RigidBodyType::Dynamic, glm::vec3(0.0f, 1.0f, 0.0f), false, false);
        world.CreateBodiesFromScene(scene);
        HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(0));
        e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
        world.SyncSceneToPhysics(scene);
        HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(world.HasBody(e), "body created when collider added");
        world.Shutdown();
    }
}
