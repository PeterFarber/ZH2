#include "Test.hpp"

#include <filesystem>
#include <fstream>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"

using namespace Hockey;
namespace fs = std::filesystem;

void RunPhysicsComponentTests() {
    HockeyTest::BeginSuite("PhysicsComponentTests");

    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterPhysicsComponents();

    // Defaults.
    RigidBodyComponent rb;
    HK_CHECK_MSG(rb.type == RigidBodyType::Static, "rigid body defaults to Static");
    HK_CHECK_MSG(rb.collisionDetection == CollisionDetectionMode::Discrete,
                 "rigid body defaults to discrete collision detection");
    HK_CHECK_NEAR(rb.mass, 1.0f, 1e-6);
    HK_CHECK_MSG(rb.useGravity, "rigid body uses gravity by default");
    HK_CHECK_MSG(rb.layer == PhysicsLayer::Static, "rigid body default layer Static");
    HK_CHECK_MSG(rb.materialName == "Default", "rigid body default material");

    BoxColliderComponent box;
    HK_CHECK_NEAR(box.halfExtents.x, 0.5f, 1e-6);
    HK_CHECK_MSG(!box.isTrigger, "box not trigger by default");
    SphereColliderComponent sphere;
    HK_CHECK_NEAR(sphere.radius, 0.5f, 1e-6);
    CapsuleColliderComponent capsule;
    HK_CHECK_NEAR(capsule.halfHeight, 1.0f, 1e-6);
    TriggerComponent trigger;
    HK_CHECK_MSG(trigger.detectPuck, "trigger detects puck by default");

    // Metadata registered.
    const ComponentRegistry& registry = ComponentRegistry::Get();
    HK_CHECK_MSG(registry.FindByName("RigidBodyComponent") != nullptr, "rigid body metadata registered");
    HK_CHECK_MSG(registry.FindByName("BoxColliderComponent") != nullptr, "box collider metadata registered");
    HK_CHECK_MSG(registry.FindByName("SphereColliderComponent") != nullptr, "sphere collider metadata registered");
    HK_CHECK_MSG(registry.FindByName("CapsuleColliderComponent") != nullptr, "capsule collider metadata registered");
    HK_CHECK_MSG(registry.FindByName("CylinderColliderComponent") != nullptr, "cylinder collider metadata registered");
    HK_CHECK_MSG(registry.FindByName("MeshColliderComponent") != nullptr, "mesh collider metadata registered");
    HK_CHECK_MSG(registry.FindByName("TriggerComponent") != nullptr, "trigger metadata registered");
    HK_CHECK_MSG(registry.FindByName("CharacterControllerComponent") != nullptr,
                 "character controller metadata registered");

    if (const ComponentMetadata* md = registry.FindByName("RigidBodyComponent")) {
        HK_CHECK_MSG(md->addable, "rigid body addable");
        HK_CHECK_MSG(!md->fields.empty(), "rigid body has fields");
        bool foundCollisionDetection = false;
        for (const FieldMetadata& field : md->fields) {
            if (field.name == "CollisionDetection") {
                foundCollisionDetection = true;
                HK_CHECK_MSG(field.type == FieldType::Enum, "collision detection metadata is enum");
                HK_CHECK_MSG(field.enumNames.size() == 2, "collision detection exposes discrete and continuous");
            }
        }
        HK_CHECK_MSG(foundCollisionDetection, "rigid body exposes collision detection metadata");
    }

    // RigidBodyType + collision detection + layer string round-trips.
    RigidBodyType parsedType = RigidBodyType::Static;
    HK_CHECK_MSG(RigidBodyTypeFromString("Dynamic", parsedType) && parsedType == RigidBodyType::Dynamic,
                 "rigid body type parses");
    HK_CHECK_MSG(std::string(RigidBodyTypeToString(RigidBodyType::Kinematic)) == "Kinematic",
                 "rigid body type stringifies");
    CollisionDetectionMode parsedCollision = CollisionDetectionMode::Discrete;
    HK_CHECK_MSG(CollisionDetectionModeFromString("Continuous", parsedCollision) &&
                     parsedCollision == CollisionDetectionMode::Continuous,
                 "collision detection mode parses");
    HK_CHECK_MSG(std::string(CollisionDetectionModeToString(CollisionDetectionMode::Discrete)) == "Discrete",
                 "collision detection mode stringifies");

    // Serialization round-trip through a real scene file.
    const fs::path workspace = Paths::TempFile("physics_components_ws");
    FileSystem::Remove(workspace);
    FileSystem::CreateDirectories(workspace);
    const fs::path scenePath = workspace / "physics.scene.yaml";

    {
        Scene scene("PhysicsRoundTrip");
        Entity e = scene.CreateEntity("PuckBody");
        RigidBodyComponent body;
        body.type = RigidBodyType::Dynamic;
        body.mass = 0.17f;
        body.useGravity = false;
        body.collisionDetection = CollisionDetectionMode::Continuous;
        body.layer = PhysicsLayer::Puck;
        body.materialName = "PuckRubber";
        body.linearDamping = 0.02f;
        body.lockRotationX = true;
        body.initialLinearVelocity = glm::vec3(1.0f, 0.0f, -2.0f);
        e.AddComponent<RigidBodyComponent>(body);

        SphereColliderComponent col;
        col.radius = 0.12f;
        col.isTrigger = false;
        e.AddComponent<SphereColliderComponent>(col);

        Entity goal = scene.CreateEntity("GoalTrigger");
        BoxColliderComponent gbox;
        gbox.halfExtents = glm::vec3(1.0f, 1.2f, 0.3f);
        gbox.isTrigger = true;
        goal.AddComponent<BoxColliderComponent>(gbox);
        TriggerComponent tr;
        tr.tag = "Goal";
        tr.detectPlayers = false;
        goal.AddComponent<TriggerComponent>(tr);

        SceneSerializer serializer(scene);
        HK_CHECK_MSG(static_cast<bool>(serializer.Serialize(scenePath)), "scene with physics serializes");
    }

    {
        Scene scene("Loaded");
        SceneSerializer serializer(scene);
        HK_CHECK_MSG(static_cast<bool>(serializer.Deserialize(scenePath)), "scene with physics deserializes");

        Entity puck = scene.FindEntityByName("PuckBody");
        HK_CHECK_MSG(puck.IsValid(), "puck entity restored");
        if (puck.IsValid()) {
            HK_CHECK_MSG(puck.HasComponent<RigidBodyComponent>(), "rigid body restored");
            HK_CHECK_MSG(puck.HasComponent<SphereColliderComponent>(), "sphere collider restored");
            const auto& body = puck.GetComponent<RigidBodyComponent>();
            HK_CHECK_MSG(body.type == RigidBodyType::Dynamic, "rigid body type restored");
            HK_CHECK_MSG(body.collisionDetection == CollisionDetectionMode::Continuous,
                         "collision detection mode restored");
            HK_CHECK_NEAR(body.mass, 0.17f, 1e-5);
            HK_CHECK_MSG(!body.useGravity, "use gravity restored");
            HK_CHECK_MSG(body.layer == PhysicsLayer::Puck, "layer restored");
            HK_CHECK_MSG(body.materialName == "PuckRubber", "material restored");
            HK_CHECK_MSG(body.lockRotationX, "lock rotation restored");
            HK_CHECK_NEAR(body.initialLinearVelocity.z, -2.0f, 1e-5);
            HK_CHECK_NEAR(puck.GetComponent<SphereColliderComponent>().radius, 0.12f, 1e-5);
        }

        Entity goal = scene.FindEntityByName("GoalTrigger");
        HK_CHECK_MSG(goal.IsValid() && goal.HasComponent<TriggerComponent>(), "trigger restored");
        if (goal.IsValid() && goal.HasComponent<BoxColliderComponent>()) {
            HK_CHECK_MSG(goal.GetComponent<BoxColliderComponent>().isTrigger, "box trigger flag restored");
            HK_CHECK_MSG(goal.GetComponent<TriggerComponent>().tag == "Goal", "trigger tag restored");
            HK_CHECK_MSG(!goal.GetComponent<TriggerComponent>().detectPlayers, "trigger detect flag restored");
        }
    }

    {
        const fs::path legacyPath = workspace / "legacy_puck.scene.yaml";
        std::ofstream legacy(legacyPath);
        legacy << R"(Scene:
  Name: LegacyPuck
  Mode: Edit
  Version: 1
  RootOrder:
    - 9001
Entities:
  - Entity: 9001
    NameComponent:
      Name: Legacy Puck
    ObjectSettingsComponent:
      Tag: Untagged
      Layer: Default
      Static: false
    ActiveComponent:
      Active: true
    TransformComponent:
      Position: [0, 0.1, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    ChildrenComponent:
      Children: []
    PuckComponent:
      StartsInPlay: true
    RigidBodyComponent:
      Type: Dynamic
      Mass: 0.17
      UseGravity: true
      AllowSleeping: true
      Layer: Puck
      Material: PuckRubber
    SphereColliderComponent:
      Radius: 0.1
      Offset: [0, 0, 0]
      IsTrigger: false
)";
        legacy.close();

        Scene legacyScene("LegacyLoaded");
        SceneSerializer legacySerializer(legacyScene);
        HK_CHECK_MSG(static_cast<bool>(legacySerializer.Deserialize(legacyPath)), "legacy puck scene deserializes");
        Entity legacyPuck = legacyScene.FindEntityByName("Legacy Puck");
        HK_CHECK_MSG(legacyPuck.IsValid() && legacyPuck.HasComponent<RigidBodyComponent>(),
                     "legacy puck body restored");
        if (legacyPuck.IsValid() && legacyPuck.HasComponent<RigidBodyComponent>()) {
            HK_CHECK_MSG(legacyPuck.GetComponent<RigidBodyComponent>().collisionDetection ==
                             CollisionDetectionMode::Continuous,
                         "legacy dynamic puck body migrates to continuous collision detection");
        }
    }

    FileSystem::Remove(workspace);
}
