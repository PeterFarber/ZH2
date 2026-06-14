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

Entity AddBody(Scene& scene, PhysicsWorld& world, const char* name, RigidBodyType type, const glm::vec3& pos,
               bool gravity = true) {
    Entity e = scene.CreateEntity(name);
    e.GetComponent<TransformComponent>().localPosition = pos;

    RigidBodyComponent rb;
    rb.type = type;
    rb.useGravity = gravity;
    rb.layer = type == RigidBodyType::Static ? PhysicsLayer::Static : PhysicsLayer::Puck;
    e.AddComponent<RigidBodyComponent>(rb);
    e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});

    world.CreateBody(e);
    return e;
}

void StepWorld(PhysicsWorld& world, int steps) {
    for (int i = 0; i < steps; ++i) {
        world.Step(1.0f / 60.0f);
    }
}

} // namespace

void RunRigidBodyTests() {
    HockeyTest::BeginSuite("RigidBodyTests");

    Physics::Init();

    PhysicsWorld world;
    world.Init(MakeDefaultPhysicsSettings());

    Scene scene("RigidBodies");

    // Static body does not move from gravity.
    Entity staticBody = AddBody(scene, world, "Static", RigidBodyType::Static, glm::vec3(0.0f, 5.0f, 0.0f));
    StepWorld(world, 60);
    glm::vec3 staticPos;
    HK_CHECK_MSG(world.GetBodyPosition(staticBody, staticPos), "static body position queryable");
    HK_CHECK_NEAR(staticPos.y, 5.0f, 1e-3);

    // Dynamic body moves under gravity.
    Entity dynamicBody = AddBody(scene, world, "Falling", RigidBodyType::Dynamic, glm::vec3(0.0f, 10.0f, 0.0f));
    StepWorld(world, 60);
    glm::vec3 fallPos;
    world.GetBodyPosition(dynamicBody, fallPos);
    HK_CHECK_MSG(fallPos.y < 9.9f, "dynamic body falls under gravity");

    // Kinematic body follows set transform.
    Entity kinematicBody = AddBody(scene, world, "Kinematic", RigidBodyType::Kinematic, glm::vec3(0.0f, 0.0f, 0.0f));
    world.SetBodyTransform(kinematicBody, glm::vec3(1.0f, 2.0f, 3.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    StepWorld(world, 1);
    glm::vec3 kinPos;
    world.GetBodyPosition(kinematicBody, kinPos);
    HK_CHECK_NEAR(kinPos.x, 1.0f, 1e-2);
    HK_CHECK_NEAR(kinPos.y, 2.0f, 1e-2);
    HK_CHECK_NEAR(kinPos.z, 3.0f, 1e-2);

    // Linear velocity set/get.
    Entity velocityBody =
        AddBody(scene, world, "Velocity", RigidBodyType::Dynamic, glm::vec3(0.0f, 50.0f, 0.0f), false);
    world.SetLinearVelocity(velocityBody, glm::vec3(0.0f, 4.0f, 0.0f));
    glm::vec3 vel = world.GetLinearVelocity(velocityBody);
    HK_CHECK_NEAR(vel.y, 4.0f, 1e-3);

    // Impulse changes velocity (mass defaults to 1, so dv == impulse).
    Entity impulseBody = AddBody(scene, world, "Impulse", RigidBodyType::Dynamic, glm::vec3(0.0f, 50.0f, 0.0f), false);
    world.AddImpulse(impulseBody, glm::vec3(0.0f, 10.0f, 0.0f));
    glm::vec3 impulseVel = world.GetLinearVelocity(impulseBody);
    HK_CHECK_MSG(impulseVel.y > 5.0f, "impulse increases velocity");

    // Locked axes restrict motion.
    Entity lockedEntity = scene.CreateEntity("Locked");
    lockedEntity.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    RigidBodyComponent lockedRb;
    lockedRb.type = RigidBodyType::Dynamic;
    lockedRb.useGravity = false;
    lockedRb.layer = PhysicsLayer::Puck;
    lockedRb.lockTranslationX = true;
    lockedRb.initialLinearVelocity = glm::vec3(5.0f, 0.0f, 0.0f);
    lockedEntity.AddComponent<RigidBodyComponent>(lockedRb);
    lockedEntity.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    world.CreateBody(lockedEntity);
    StepWorld(world, 30);
    glm::vec3 lockedPos;
    world.GetBodyPosition(lockedEntity, lockedPos);
    HK_CHECK_NEAR(lockedPos.x, 0.0f, 1e-2);

    world.Shutdown();
}
