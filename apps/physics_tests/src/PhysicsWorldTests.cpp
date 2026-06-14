#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

Entity MakeBox(Scene& scene, const char* name, RigidBodyType type, const glm::vec3& pos) {
    Entity e = scene.CreateEntity(name);
    e.GetComponent<TransformComponent>().localPosition = pos;

    RigidBodyComponent rb;
    rb.type = type;
    rb.layer = type == RigidBodyType::Static ? PhysicsLayer::Static : PhysicsLayer::Puck;
    e.AddComponent<RigidBodyComponent>(rb);

    e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    return e;
}

} // namespace

void RunPhysicsWorldTests() {
    HockeyTest::BeginSuite("PhysicsWorldTests");

    Physics::Init();

    // Create world.
    PhysicsWorld world;
    HK_CHECK_MSG(static_cast<bool>(world.Init(MakeDefaultPhysicsSettings())), "physics world initialises");
    HK_CHECK_MSG(world.IsInitialized(), "world reports initialised");

    // Set gravity.
    world.SetGravity(glm::vec3(0.0f, -20.0f, 0.0f));
    HK_CHECK_NEAR(world.GetGravity().y, -20.0f, 1e-4);

    // Step world (no bodies, must not crash).
    world.Step(1.0f / 60.0f);
    HK_CHECK_MSG(true, "empty world steps");

    // Create bodies from scene.
    Scene scene("World");
    Entity dynamicBody = MakeBox(scene, "Dynamic", RigidBodyType::Dynamic, glm::vec3(0.0f, 5.0f, 0.0f));
    Entity staticBody = MakeBox(scene, "Static", RigidBodyType::Static, glm::vec3(3.0f, 0.0f, 0.0f));

    world.CreateBodiesFromScene(scene);
    HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(2));
    HK_CHECK_MSG(world.HasBody(dynamicBody), "dynamic body created");
    HK_CHECK_MSG(world.HasBody(staticBody), "static body created");

    // Body mapping entity <-> physics handle works.
    PhysicsBodyHandle handle = world.CreateBody(dynamicBody); // already exists -> returns same handle
    HK_CHECK_MSG(handle.IsValid(), "existing body returns valid handle");

    Entity noBodyEntity = scene.CreateEntity("NoCollider");
    noBodyEntity.AddComponent<RigidBodyComponent>(RigidBodyComponent{});
    PhysicsBodyHandle invalid = world.CreateBody(noBodyEntity);
    HK_CHECK_MSG(!invalid.IsValid(), "entity without collider yields invalid handle");
    HK_CHECK_MSG(!world.HasBody(noBodyEntity), "entity without collider has no body");

    // Destroy bodies.
    world.DestroyBodies();
    HK_CHECK_EQ(world.BodyCount(), static_cast<std::size_t>(0));
    HK_CHECK_MSG(!world.HasBody(dynamicBody), "bodies destroyed");

    world.Shutdown();
    HK_CHECK_MSG(!world.IsInitialized(), "world shuts down");
}
