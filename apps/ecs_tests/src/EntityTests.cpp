#include "Test.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

using namespace Hockey;

void RunEntityTests() {
    HockeyTest::BeginSuite("EntityTests");

    Scene scene("Test");

    Entity a = scene.CreateEntity("Alpha");
    HK_CHECK(a.IsValid());
    HK_CHECK(a.HasComponent<IDComponent>());
    HK_CHECK(a.HasComponent<NameComponent>());
    HK_CHECK(a.HasComponent<ActiveComponent>());
    HK_CHECK(a.HasComponent<TransformComponent>());
    HK_CHECK(a.HasComponent<ChildrenComponent>());
    HK_CHECK_EQ(a.GetName(), std::string("Alpha"));
    HK_CHECK(a.GetUUID().IsValid());

    Entity b = scene.CreateEntity("Beta");
    HK_CHECK(a.GetUUID() != b.GetUUID());

    const UUID fixed(1234567890123ULL);
    Entity c = scene.CreateEntityWithUUID(fixed, "Gamma");
    HK_CHECK(c.GetUUID() == fixed);

    HK_CHECK(scene.FindEntityByUUID(fixed) == c);
    HK_CHECK(scene.FindEntityByName("Beta") == b);
    HK_CHECK(!scene.FindEntityByName("DoesNotExist").IsValid());

    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(3));

    const UUID bId = b.GetUUID();
    scene.DestroyEntity(b);
    HK_CHECK(!scene.ContainsUUID(bId));
    HK_CHECK(!scene.FindEntityByUUID(bId).IsValid());
    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(2));

    // Recursive destroy removes children too.
    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");
    scene.SetParent(child, parent);
    const UUID parentId = parent.GetUUID();
    const UUID childId = child.GetUUID();
    scene.DestroyEntityRecursive(parent);
    HK_CHECK(!scene.ContainsUUID(parentId));
    HK_CHECK(!scene.ContainsUUID(childId));

    // Duplicate creates new UUIDs and copies component data.
    Entity src = scene.CreateEntity("Source");
    src.GetComponent<TransformComponent>().localPosition = glm::vec3(5.0f, 6.0f, 7.0f);
    src.AddComponent<TeamComponent>(TeamComponent{Team::Home});
    Entity srcChild = scene.CreateEntity("SourceChild");
    scene.SetParent(srcChild, src);

    Entity dup = scene.DuplicateEntity(src);
    HK_CHECK(dup.IsValid());
    HK_CHECK(dup.GetUUID() != src.GetUUID());
    HK_CHECK_EQ(dup.GetName(), std::string("Source"));
    HK_CHECK(dup.HasComponent<TeamComponent>());
    HK_CHECK(dup.GetComponent<TeamComponent>().team == Team::Home);
    HK_CHECK_NEAR(dup.GetComponent<TransformComponent>().localPosition.x, 5.0f, 1e-4);
    HK_CHECK_EQ(scene.GetChildren(dup).size(), static_cast<std::size_t>(1));
    HK_CHECK(scene.GetChildren(dup)[0].GetUUID() != srcChild.GetUUID());
}
