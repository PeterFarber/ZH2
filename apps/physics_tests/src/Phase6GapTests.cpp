#include "Test.hpp"

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsLayer.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsMeshProvider.hpp"
#include "Hockey/Physics/PhysicsShape.hpp"
#include "Hockey/Physics/PhysicsValidation.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

using namespace Hockey;
namespace fs = std::filesystem;

namespace {

Entity MakeStaticFloor(Scene& scene, const glm::vec3& pos, const glm::vec3& halfExtents) {
    Entity e = scene.CreateEntity("Floor");
    e.GetComponent<TransformComponent>().localPosition = pos;
    RigidBodyComponent rb;
    rb.type = RigidBodyType::Static;
    rb.layer = PhysicsLayer::Rink;
    e.AddComponent<RigidBodyComponent>(rb);
    BoxColliderComponent box;
    box.halfExtents = halfExtents;
    e.AddComponent<BoxColliderComponent>(box);
    return e;
}

// Axis-aligned box as a triangle mesh (8 corners, 12 triangles), used to feed
// the mesh-collider provider without needing the asset pipeline.
PhysicsMeshData MakeBoxMesh(const glm::vec3& halfExtents) {
    const glm::vec3& h = halfExtents;
    PhysicsMeshData data;
    data.vertices = {
        {-h.x, -h.y, -h.z}, {h.x, -h.y, -h.z}, {h.x, h.y, -h.z}, {-h.x, h.y, -h.z},
        {-h.x, -h.y, h.z},  {h.x, -h.y, h.z},  {h.x, h.y, h.z},  {-h.x, h.y, h.z},
    };
    data.indices = {
        0, 1, 2, 0, 2, 3, // -Z
        4, 6, 5, 4, 7, 6, // +Z
        0, 5, 1, 0, 4, 5, // -Y
        3, 2, 6, 3, 6, 7, // +Y
        0, 3, 7, 0, 7, 4, // -X
        1, 5, 6, 1, 6, 2, // +X
    };
    return data;
}

Entity MakeDynamicPuck(Scene& scene, const glm::vec3& pos, float radius, const char* material) {
    Entity e = scene.CreateEntity("Puck");
    e.GetComponent<TransformComponent>().localPosition = pos;
    RigidBodyComponent rb;
    rb.type = RigidBodyType::Dynamic;
    rb.layer = PhysicsLayer::Puck;
    rb.materialName = material;
    rb.allowSleeping = false;
    e.AddComponent<RigidBodyComponent>(rb);
    SphereColliderComponent sphere;
    sphere.radius = radius;
    e.AddComponent<SphereColliderComponent>(sphere);
    return e;
}

} // namespace

void RunShapeCastTests() {
    HockeyTest::BeginSuite("ShapeCastTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    PhysicsWorld world;
    world.Init(MakeDefaultPhysicsSettings());

    Scene scene("ShapeCast");
    Entity floor = MakeStaticFloor(scene, glm::vec3(0.0f), glm::vec3(2.0f, 1.0f, 2.0f)); // top at y = 1
    world.CreateBodiesFromScene(scene);
    const UUID floorId = floor.GetUUID();

    // Sphere sweep straight down hits the floor top.
    {
        ShapeCastRequest req;
        req.shape = ShapeCastRequest::Shape::Sphere;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.radius = 0.5f;
        req.maxDistance = 10.0f;
        ShapeCastHit hit;
        const bool ok = world.ShapeCast(req, hit);
        HK_CHECK_MSG(ok, "sphere cast hits the floor");
        HK_CHECK_MSG(hit.entity == floorId, "sphere cast reports the floor entity");
        // Sphere of radius 0.5 from y=5 stops with its centre at y≈1.5, so it
        // travels roughly 3.5 units before contact.
        HK_CHECK_NEAR(hit.distance, 3.5f, 0.2);
    }

    // Casting away from geometry misses.
    {
        ShapeCastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        req.radius = 0.5f;
        req.maxDistance = 10.0f;
        ShapeCastHit hit;
        HK_CHECK_MSG(!world.ShapeCast(req, hit), "sphere cast misses when pointing away");
    }

    // Layer mask excludes the floor (Rink layer).
    {
        ShapeCastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.radius = 0.5f;
        req.maxDistance = 10.0f;
        req.layerMask = ~(1u << static_cast<std::uint32_t>(PhysicsLayer::Rink));
        ShapeCastHit hit;
        HK_CHECK_MSG(!world.ShapeCast(req, hit), "sphere cast respects layer mask");
    }

    // Capsule sweep straight down hits the floor top (player-shaped probe).
    {
        ShapeCastRequest req;
        req.shape = ShapeCastRequest::Shape::Capsule;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.radius = 0.4f;
        req.halfHeight = 0.8f;
        req.maxDistance = 10.0f;
        ShapeCastHit hit;
        const bool ok = world.ShapeCast(req, hit);
        HK_CHECK_MSG(ok, "capsule cast hits the floor");
        HK_CHECK_MSG(hit.entity == floorId, "capsule cast reports the floor entity");
        // Capsule half-height 0.8 + radius 0.4 = 1.2 bottom reach; from y=5 the
        // lowest point meets the floor top (y=1) after ~2.8 units of travel.
        HK_CHECK_NEAR(hit.distance, 2.8f, 0.3);
    }

    // A degenerate capsule (zero radius/half-height) is rejected.
    {
        ShapeCastRequest req;
        req.shape = ShapeCastRequest::Shape::Capsule;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.radius = 0.0f;
        req.halfHeight = 0.0f;
        req.maxDistance = 10.0f;
        ShapeCastHit hit;
        HK_CHECK_MSG(!world.ShapeCast(req, hit), "degenerate capsule cast is rejected");
    }

    world.Shutdown();
}

void RunMaterialCombineModeTests() {
    HockeyTest::BeginSuite("MaterialCombineModeTests");

    PhysicsMaterial a;
    a.friction = 0.4f;
    a.restitution = 0.2f;
    PhysicsMaterial b;
    b.friction = 0.9f;
    b.restitution = 0.6f;

    // Default (no flags): geometric-mean friction, averaged restitution.
    HK_CHECK_NEAR(CombineFriction(a, b), std::sqrt(0.4f * 0.9f), 1e-5);
    HK_CHECK_NEAR(CombineRestitution(a, b), 0.5f * (0.2f + 0.6f), 1e-5);

    // Multiply-friction flag on either material switches to the product.
    PhysicsMaterial aMul = a;
    aMul.combineFrictionMultiply = true;
    HK_CHECK_NEAR(CombineFriction(aMul, b), 0.4f * 0.9f, 1e-5);
    HK_CHECK_NEAR(CombineFriction(b, aMul), 0.4f * 0.9f, 1e-5);

    // Max-restitution flag on either material switches to the bouncier surface.
    PhysicsMaterial bMax = b;
    bMax.combineRestitutionMax = true;
    HK_CHECK_NEAR(CombineRestitution(a, bMax), 0.6f, 1e-5);
    HK_CHECK_NEAR(CombineRestitution(bMax, a), 0.6f, 1e-5);

    // Built-in materials keep the historical max-restitution behaviour.
    const std::vector<PhysicsMaterial> builtIns = MakeBuiltInMaterials();
    bool allMax = !builtIns.empty();
    for (const PhysicsMaterial& m : builtIns) {
        allMax = allMax && m.combineRestitutionMax;
    }
    HK_CHECK_MSG(allMax, "built-in materials default to max-restitution combine");
}

void RunSensorLayerTests() {
    HockeyTest::BeginSuite("SensorLayerTests");

    CollisionFilter::ResetDefaults();

    // Sensor volumes detect the moving actors...
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Sensor, PhysicsLayer::Player),
                 "sensor collides with players");
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Sensor, PhysicsLayer::Goalie),
                 "sensor collides with goalies");
    HK_CHECK_MSG(CollisionFilter::ShouldCollide(PhysicsLayer::Sensor, PhysicsLayer::Puck),
                 "sensor collides with the puck");
    // ...but never the static world (so they stay non-physical detection zones).
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Sensor, PhysicsLayer::Static),
                 "sensor ignores static geometry");
    HK_CHECK_MSG(!CollisionFilter::ShouldCollide(PhysicsLayer::Sensor, PhysicsLayer::Rink),
                 "sensor ignores the rink");
}

void RunPlayerCapsuleValidationTests() {
    HockeyTest::BeginSuite("PlayerCapsuleValidationTests");

    Physics::Init();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    auto hasWarningContaining = [](const std::vector<SceneValidationIssue>& issues, const char* needle) {
        for (const SceneValidationIssue& issue : issues) {
            if (issue.message.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    };

    // A skater set up as a physics actor but using a box collider is flagged.
    {
        Scene scene("PlayerBox");
        Entity e = scene.CreateEntity("Skater");
        e.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{PlayerRole::Skater});
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Dynamic;
        rb.layer = PhysicsLayer::Player;
        e.AddComponent<RigidBodyComponent>(rb);
        e.AddComponent<BoxColliderComponent>(BoxColliderComponent{});
        std::vector<SceneValidationIssue> issues;
        ValidatePhysicsScene(scene, issues);
        HK_CHECK_MSG(hasWarningContaining(issues, "CapsuleCollider"),
                     "non-capsule player body is flagged");
    }

    // A goalie with an invalid (zero) capsule is flagged.
    {
        Scene scene("GoalieBadCapsule");
        Entity e = scene.CreateEntity("Goalie");
        e.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{PlayerRole::Goalie});
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Dynamic;
        rb.layer = PhysicsLayer::Goalie;
        e.AddComponent<RigidBodyComponent>(rb);
        CapsuleColliderComponent cap;
        cap.radius = 0.0f;
        cap.halfHeight = 0.0f;
        e.AddComponent<CapsuleColliderComponent>(cap);
        std::vector<SceneValidationIssue> issues;
        ValidatePhysicsScene(scene, issues);
        HK_CHECK_MSG(hasWarningContaining(issues, "Goalie capsule has invalid"),
                     "invalid goalie capsule is flagged");
    }

    // A skater with a valid capsule raises no player-capsule warning.
    {
        Scene scene("PlayerCapsule");
        Entity e = scene.CreateEntity("Skater");
        e.AddComponent<PlayerRoleComponent>(PlayerRoleComponent{PlayerRole::Skater});
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Dynamic;
        rb.layer = PhysicsLayer::Player;
        rb.materialName = "PlayerBody";
        e.AddComponent<RigidBodyComponent>(rb);
        CapsuleColliderComponent cap;
        cap.radius = 0.45f;
        cap.halfHeight = 0.9f;
        e.AddComponent<CapsuleColliderComponent>(cap);
        std::vector<SceneValidationIssue> issues;
        ValidatePhysicsScene(scene, issues);
        HK_CHECK_MSG(!hasWarningContaining(issues, "CapsuleCollider") &&
                         !hasWarningContaining(issues, "capsule has invalid"),
                     "valid player capsule passes validation");
    }
}

void RunRaycastAllTests() {
    HockeyTest::BeginSuite("RaycastAllTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    PhysicsWorld world;
    world.Init(MakeDefaultPhysicsSettings());

    Scene scene("RaycastAll");
    Entity lower = MakeStaticFloor(scene, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)); // top at y = 1
    lower.SetName("Lower");
    Entity upper = scene.CreateEntity("Upper");
    upper.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 5.0f, 0.0f); // top at y = 6
    {
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Static;
        rb.layer = PhysicsLayer::Rink;
        upper.AddComponent<RigidBodyComponent>(rb);
        BoxColliderComponent box;
        box.halfExtents = glm::vec3(1.0f);
        upper.AddComponent<BoxColliderComponent>(box);
    }
    world.CreateBodiesFromScene(scene);

    const UUID lowerId = lower.GetUUID();
    const UUID upperId = upper.GetUUID();

    RaycastRequest req;
    req.origin = glm::vec3(0.0f, 10.0f, 0.0f);
    req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    req.maxDistance = 20.0f;
    const std::vector<RaycastHit> hits = world.RaycastAll(req);

    HK_CHECK_EQ(hits.size(), static_cast<std::size_t>(2));
    if (hits.size() == 2) {
        HK_CHECK_MSG(hits[0].distance <= hits[1].distance, "RaycastAll returns hits sorted by distance");
        HK_CHECK_MSG(hits[0].entity == upperId, "nearest hit is the upper box");
        HK_CHECK_MSG(hits[1].entity == lowerId, "farthest hit is the lower box");
    }

    world.Shutdown();
}

void RunTriggerDetectFlagTests() {
    HockeyTest::BeginSuite("TriggerDetectFlagTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    auto runWithDetectPuck = [](bool detectPuck) {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("TriggerFlags");

        Entity trigger = scene.CreateEntity("Goal");
        trigger.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f);
        RigidBodyComponent trb;
        trb.type = RigidBodyType::Static;
        trb.layer = PhysicsLayer::Goal;
        trigger.AddComponent<RigidBodyComponent>(trb);
        BoxColliderComponent tbox;
        tbox.halfExtents = glm::vec3(2.0f);
        tbox.isTrigger = true;
        trigger.AddComponent<BoxColliderComponent>(tbox);
        TriggerComponent tc;
        tc.detectPuck = detectPuck;
        trigger.AddComponent<TriggerComponent>(tc);

        MakeDynamicPuck(scene, glm::vec3(0.0f, 6.0f, 0.0f), 0.2f, "PuckRubber");
        world.CreateBodiesFromScene(scene);

        bool rawSawEvent = false;
        bool filteredSawEvent = false;
        for (int i = 0; i < 240; ++i) {
            world.Step(1.0f / 60.0f);
            // Filtered drain consumes the queue; mirror it into a raw flag by
            // checking whether any unfiltered event would have appeared first.
            const auto filtered = world.DrainTriggerEvents(scene);
            if (!filtered.empty()) {
                filteredSawEvent = true;
            }
        }
        // Re-run identically to observe the raw (unfiltered) stream.
        PhysicsWorld rawWorld;
        rawWorld.Init(MakeDefaultPhysicsSettings());
        Scene rawScene("TriggerFlagsRaw");
        Entity rawTrigger = rawScene.CreateEntity("Goal");
        rawTrigger.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f);
        rawTrigger.AddComponent<RigidBodyComponent>(trb);
        rawTrigger.AddComponent<BoxColliderComponent>(tbox);
        rawTrigger.AddComponent<TriggerComponent>(tc);
        MakeDynamicPuck(rawScene, glm::vec3(0.0f, 6.0f, 0.0f), 0.2f, "PuckRubber");
        rawWorld.CreateBodiesFromScene(rawScene);
        for (int i = 0; i < 240; ++i) {
            rawWorld.Step(1.0f / 60.0f);
            if (!rawWorld.DrainTriggerEvents().empty()) {
                rawSawEvent = true;
            }
        }
        world.Shutdown();
        rawWorld.Shutdown();
        return std::pair<bool, bool>{rawSawEvent, filteredSawEvent};
    };

    // The raw stream always reports the puck passing through; the filtered drain
    // suppresses it only when detectPuck is false.
    const auto [rawOff, filteredOff] = runWithDetectPuck(false);
    HK_CHECK_MSG(rawOff, "raw drain still reports the puck entering the trigger");
    HK_CHECK_MSG(!filteredOff, "detectPuck=false suppresses puck trigger events");

    const auto [rawOn, filteredOn] = runWithDetectPuck(true);
    HK_CHECK_MSG(rawOn, "raw drain reports the puck (detectPuck=true)");
    HK_CHECK_MSG(filteredOn, "detectPuck=true keeps puck trigger events");
}

void RunDeterminismTests() {
    HockeyTest::BeginSuite("DeterminismTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();

    auto simulate = [](const PhysicsSettings& settings, glm::vec3& outPos) {
        PhysicsWorld world;
        world.Init(settings);
        Scene scene("Determinism");
        MakeStaticFloor(scene, glm::vec3(0.0f), glm::vec3(4.0f, 0.5f, 4.0f));
        Entity puck = MakeDynamicPuck(scene, glm::vec3(0.3f, 4.0f, -0.2f), 0.2f, "PuckRubber");
        world.CreateBodiesFromScene(scene);
        for (int i = 0; i < 180; ++i) {
            world.Step(1.0f / 60.0f);
        }
        world.SyncPhysicsToScene(scene);
        world.GetBodyPosition(puck, outPos);
        world.Shutdown();
    };

    PhysicsSettings deterministic = MakeDefaultPhysicsSettings();
    deterministic.deterministicMode = true;

    glm::vec3 runA(0.0f);
    glm::vec3 runB(0.0f);
    simulate(deterministic, runA);
    simulate(deterministic, runB);

    HK_CHECK_MSG(runA.y < 3.0f, "puck actually fell during the simulation");
    HK_CHECK_NEAR(runA.x, runB.x, 1e-4);
    HK_CHECK_NEAR(runA.y, runB.y, 1e-4);
    HK_CHECK_NEAR(runA.z, runB.z, 1e-4);

    // integrationSubsteps > 1 still produces a valid (falling) simulation.
    PhysicsSettings substepped = MakeDefaultPhysicsSettings();
    substepped.integrationSubsteps = 4;
    glm::vec3 subPos(0.0f);
    simulate(substepped, subPos);
    HK_CHECK_MSG(subPos.y < 3.0f, "substepped simulation still advances");
}

void RunPhysicsMaterialComponentTests() {
    HockeyTest::BeginSuite("PhysicsMaterialComponentTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();
    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterPhysicsComponents();

    // Metadata registered.
    HK_CHECK_MSG(ComponentRegistry::Get().FindByName("PhysicsMaterialComponent") != nullptr,
                 "physics material component metadata registered");

    // The override changes simulated behaviour: a puck whose rigid body uses the
    // low-restitution Ice material but carries a PhysicsMaterialComponent for the
    // bouncier PuckRubber should rebound faster than one without the override.
    auto maxRebound = [](bool withOverride) {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("MaterialBounce");
        MakeStaticFloor(scene, glm::vec3(0.0f), glm::vec3(4.0f, 0.5f, 4.0f)); // top at y = 0.5
        Entity puck = MakeDynamicPuck(scene, glm::vec3(0.0f, 3.0f, 0.0f), 0.2f, "Ice");
        if (withOverride) {
            puck.AddComponent<PhysicsMaterialComponent>(PhysicsMaterialComponent{"PuckRubber"});
        }
        world.CreateBodiesFromScene(scene);

        float maxUpVelocity = 0.0f;
        for (int i = 0; i < 240; ++i) {
            world.Step(1.0f / 60.0f);
            const float vy = world.GetLinearVelocity(puck).y;
            if (vy > maxUpVelocity) {
                maxUpVelocity = vy;
            }
        }
        world.Shutdown();
        return maxUpVelocity;
    };

    const float reboundPlain = maxRebound(false);
    const float reboundOverridden = maxRebound(true);
    HK_CHECK_MSG(reboundOverridden > reboundPlain + 0.1f,
                 "PhysicsMaterialComponent override increases restitution (puck bounces higher)");

    // Serialization round-trip through a scene file.
    const fs::path workspace = Paths::TempFile("physics_material_ws");
    FileSystem::Remove(workspace);
    FileSystem::CreateDirectories(workspace);
    const fs::path scenePath = workspace / "material.scene.yaml";
    {
        Scene scene("MaterialRoundTrip");
        Entity e = scene.CreateEntity("Body");
        e.AddComponent<RigidBodyComponent>(RigidBodyComponent{});
        e.AddComponent<PhysicsMaterialComponent>(PhysicsMaterialComponent{"Boards"});
        SceneSerializer serializer(scene);
        HK_CHECK_MSG(static_cast<bool>(serializer.Serialize(scenePath)), "scene with material component serializes");
    }
    {
        Scene scene("Loaded");
        SceneSerializer serializer(scene);
        HK_CHECK_MSG(static_cast<bool>(serializer.Deserialize(scenePath)), "scene with material component deserializes");
        Entity e = scene.FindEntityByName("Body");
        HK_CHECK_MSG(e.IsValid() && e.HasComponent<PhysicsMaterialComponent>(), "material component restored");
        if (e.IsValid() && e.HasComponent<PhysicsMaterialComponent>()) {
            HK_CHECK_MSG(e.GetComponent<PhysicsMaterialComponent>().materialName == "Boards",
                         "material override name restored");
        }
    }
    FileSystem::Remove(workspace);
}

void RunMeshColliderProviderTests() {
    HockeyTest::BeginSuite("MeshColliderProviderTests");

    Physics::Init();
    CollisionFilter::ResetDefaults();
    PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    constexpr std::uint64_t kFloorMeshId = 4242;
    const glm::vec3 floorHalf(2.0f, 0.25f, 2.0f);

    // Without a provider installed, a mesh collider cannot resolve geometry.
    {
        PhysicsMeshRegistry::Get().Clear();
        MeshColliderComponent mesh;
        mesh.meshAsset = kFloorMeshId;
        const ShapeBuildResult result = TryBuildMeshShape(mesh);
        HK_CHECK_MSG(!result.success, "mesh collider fails without a provider");
        HK_CHECK_MSG(result.error.find("provider") != std::string::npos, "failure mentions the missing provider");
    }

    // Install a provider that serves a unit-ish box for the known id only.
    PhysicsMeshRegistry::Get().SetProvider([&](std::uint64_t id, PhysicsMeshData& out) -> bool {
        if (id != kFloorMeshId) {
            return false;
        }
        out = MakeBoxMesh(floorHalf);
        return true;
    });

    // Convex mesh collider builds a hull with positive volume.
    {
        MeshColliderComponent mesh;
        mesh.meshAsset = kFloorMeshId;
        mesh.convex = true;
        const ShapeBuildResult result = TryBuildMeshShape(mesh);
        HK_CHECK_MSG(result.success, "convex mesh collider builds a shape");
        HK_CHECK_MSG(result.volume > 0.0f, "convex hull has volume");
    }

    // Concave (triangle-mesh) collider builds and spans the authored extents.
    {
        MeshColliderComponent mesh;
        mesh.meshAsset = kFloorMeshId;
        mesh.convex = false;
        const ShapeBuildResult result = TryBuildMeshShape(mesh);
        HK_CHECK_MSG(result.success, "concave mesh collider builds a shape");
        HK_CHECK_MSG((result.boundsMax - result.boundsMin).x > 1.0f, "mesh bounds span the box width");
    }

    // Unknown asset id is rejected even with a provider present.
    {
        MeshColliderComponent mesh;
        mesh.meshAsset = 999999;
        const ShapeBuildResult result = TryBuildMeshShape(mesh);
        HK_CHECK_MSG(!result.success, "unknown mesh id fails to resolve");
    }

    // End-to-end: a static entity with a mesh collider produces a body that a
    // downward ray can hit.
    {
        PhysicsWorld world;
        world.Init(MakeDefaultPhysicsSettings());
        Scene scene("MeshFloor");
        Entity floor = scene.CreateEntity("MeshFloor");
        floor.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f);
        RigidBodyComponent rb;
        rb.type = RigidBodyType::Static;
        rb.layer = PhysicsLayer::Rink;
        floor.AddComponent<RigidBodyComponent>(rb);
        MeshColliderComponent mesh;
        mesh.meshAsset = kFloorMeshId;
        mesh.convex = true; // works for both static and dynamic bodies
        floor.AddComponent<MeshColliderComponent>(mesh);

        world.CreateBodiesFromScene(scene);
        HK_CHECK_MSG(world.HasBody(floor), "mesh-collider entity gets a body");

        RaycastRequest req;
        req.origin = glm::vec3(0.0f, 5.0f, 0.0f);
        req.direction = glm::vec3(0.0f, -1.0f, 0.0f);
        req.maxDistance = 10.0f;
        RaycastHit hit;
        const bool ok = world.Raycast(req, hit);
        HK_CHECK_MSG(ok, "ray hits the mesh-collider floor");
        HK_CHECK_MSG(hit.entity == floor.GetUUID(), "ray reports the mesh-collider entity");
        // Box top sits at y = 0.25, so a ray from y = 5 travels ~4.75 units.
        HK_CHECK_NEAR(hit.distance, 4.75f, 0.2);
        world.Shutdown();
    }

    // Clean up the process-global provider so later suites see no provider.
    PhysicsMeshRegistry::Get().Clear();
}
