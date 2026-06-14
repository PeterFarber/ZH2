#include "Test.hpp"

#include <memory>

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneMode.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsScene.hpp"
#include "Hockey/Physics/PhysicsSystem.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;

namespace {

Entity MakeDynamicBox(Scene& scene, const char* name, const glm::vec3& pos) {
    Entity e = scene.CreateEntity(name);
    e.GetComponent<TransformComponent>().localPosition = pos;
    RigidBodyComponent rb;
    rb.type = RigidBodyType::Dynamic;
    rb.useGravity = true;
    rb.layer = PhysicsLayer::Puck;
    e.AddComponent<RigidBodyComponent>(rb);
    e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
    return e;
}

} // namespace

void RunHeadlessServerPhysicsTests() {
    HockeyTest::BeginSuite("HeadlessServerPhysicsTests");

    Physics::Init();

    // PhysicsScene drives a full sync/step/writeback cycle headlessly.
    {
        PhysicsScene physicsScene;
        Scene scene("ServerScene");
        scene.SetMode(SceneMode::Server);
        Entity box = MakeDynamicBox(scene, "Box", glm::vec3(0.0f, 20.0f, 0.0f));

        Status status = physicsScene.Init(scene, MakeDefaultPhysicsSettings());
        HK_CHECK_MSG(status.success, "physics scene initialises");
        HK_CHECK_MSG(physicsScene.IsInitialized(), "physics scene reports initialised");

        physicsScene.OnSimulationStart(scene);
        HK_CHECK_EQ(physicsScene.World().BodyCount(), static_cast<std::size_t>(1));

        for (int i = 0; i < 60; ++i) {
            physicsScene.OnFixedUpdate(scene, 1.0f / 60.0f);
        }

        const float y = box.GetComponent<TransformComponent>().localPosition.y;
        HK_CHECK_MSG(y < 20.0f, "headless fixed update advances the simulation");

        physicsScene.OnSimulationStop(scene);
        HK_CHECK_EQ(physicsScene.World().BodyCount(), static_cast<std::size_t>(0));

        physicsScene.Shutdown();
        HK_CHECK_MSG(!physicsScene.IsInitialized(), "physics scene shuts down");
    }

    // PhysicsSystem runs through the Scene system lifecycle (server mode).
    {
        Scene scene("ServerSystemScene");
        scene.SetMode(SceneMode::Server);
        Entity box = MakeDynamicBox(scene, "Box", glm::vec3(0.0f, 20.0f, 0.0f));

        auto system = std::make_unique<PhysicsSystem>(MakeDefaultPhysicsSettings());
        PhysicsSystem* systemPtr = system.get();
        scene.AddSystem(std::move(system));
        HK_CHECK_EQ(scene.SystemCount(), static_cast<std::size_t>(1));

        scene.OnSimulationStart();
        HK_CHECK_MSG(systemPtr->IsInitialized(), "physics system initialises on start");
        HK_CHECK_EQ(systemPtr->World().BodyCount(), static_cast<std::size_t>(1));

        for (int i = 0; i < 120; ++i) {
            scene.OnFixedUpdate(1.0f / 60.0f);
        }

        const float y = box.GetComponent<TransformComponent>().localPosition.y;
        HK_CHECK_MSG(y < 19.0f, "server scene fixed tick simulates physics");

        scene.OnSimulationStop();
        HK_CHECK_EQ(systemPtr->World().BodyCount(), static_cast<std::size_t>(0));
    }

    // Edit mode does not advance the simulation unless preview is enabled.
    {
        Scene scene("EditScene");
        scene.SetMode(SceneMode::Edit);
        Entity box = MakeDynamicBox(scene, "Box", glm::vec3(0.0f, 20.0f, 0.0f));

        auto system = std::make_unique<PhysicsSystem>(MakeDefaultPhysicsSettings());
        PhysicsSystem* systemPtr = system.get();
        scene.AddSystem(std::move(system));

        scene.OnSimulationStart();
        const float startY = box.GetComponent<TransformComponent>().localPosition.y;

        for (int i = 0; i < 60; ++i) {
            scene.OnFixedUpdate(1.0f / 60.0f);
        }
        HK_CHECK_NEAR(box.GetComponent<TransformComponent>().localPosition.y, startY, 1e-3);

        // Enable preview -> simulation advances.
        systemPtr->SetEditorPreviewEnabled(true);
        for (int i = 0; i < 60; ++i) {
            scene.OnFixedUpdate(1.0f / 60.0f);
        }
        HK_CHECK_MSG(box.GetComponent<TransformComponent>().localPosition.y < startY,
                     "edit preview advances physics once enabled");

        scene.OnSimulationStop();
    }
}
