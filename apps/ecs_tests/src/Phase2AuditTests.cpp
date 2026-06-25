#include "Test.hpp"

#include <cstddef>
#include <filesystem>

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabOverride.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"

using namespace Hockey;

// Loads the shipped main scene end-to-end and checks structural integrity.
void RunMainRinkLoadTests() {
    HockeyTest::BeginSuite("MainRinkLoadTests");

    const std::filesystem::path path = Paths::RawAsset("scenes/Main.scene.yaml");
    if (!FileSystem::Exists(path)) {
        HK_CHECK_MSG(false, "Main.scene.yaml not found under data/raw/scenes");
        return;
    }

    Scene scene("Loaded");
    SceneSerializer serializer(scene);
    const Status loaded = serializer.Deserialize(path);
    HK_CHECK_MSG(static_cast<bool>(loaded), "Main.scene.yaml deserializes");
    if (!loaded) {
        return;
    }

    HK_CHECK_EQ(scene.GetName(), std::string("Main"));
    HK_CHECK_MSG(scene.EntityCount() >= static_cast<std::size_t>(8), "Main scene has expected authored content");

    // Main is a valid editor scene in the tracked baseline; local playtest
    // copies may additionally contain gameplay markers.
    HK_CHECK(scene.FindEntityByName("Camera").IsValid());
    if (scene.FindEntityByName("Puck Spawn").IsValid()) {
        HK_CHECK(scene.FindEntityByName("Home Goal").IsValid());
        HK_CHECK(scene.FindEntityByName("Away Goal").IsValid());
    }

    // The shipped scene must be structurally valid (no errors).
    const auto issues = SceneValidator::Validate(scene);
    HK_CHECK_MSG(!SceneValidator::HasErrors(issues), "Main scene validates without errors");
}

// DestroyEntity detaches children to root while preserving their world transform.
void RunDestroyWorldTransformTests() {
    HockeyTest::BeginSuite("DestroyWorldTransformTests");

    Scene scene("DestroyWorld");
    Entity parent = scene.CreateEntity("Parent");
    parent.GetComponent<TransformComponent>().localPosition = {10.0f, 0.0f, 0.0f};
    Entity child = scene.CreateEntity("Child");
    scene.SetParent(child, parent, false);
    child.GetComponent<TransformComponent>().localPosition = {5.0f, 0.0f, 0.0f};

    const glm::vec3 worldBefore = glm::vec3(scene.GetWorldTransform(child)[3]);
    HK_CHECK_NEAR(worldBefore.x, 15.0f, 0.0001f);

    scene.DestroyEntity(parent);

    // Child survives, is now a root, and keeps its world position (15,0,0).
    HK_CHECK(child.IsValid());
    HK_CHECK(!scene.GetParent(child).IsValid());
    const glm::vec3 worldAfter = glm::vec3(scene.GetWorldTransform(child)[3]);
    HK_CHECK_NEAR(worldAfter.x, 15.0f, 0.0001f);
    HK_CHECK_NEAR(worldAfter.y, 0.0f, 0.0001f);
    HK_CHECK_NEAR(worldAfter.z, 0.0f, 0.0001f);
}

// Applying prefab overrides surfaces failures for bad entities/components/fields.
void RunPrefabOverrideApplyTests() {
    HockeyTest::BeginSuite("PrefabOverrideApplyTests");

    Scene scene("Overrides");
    Entity entity = scene.CreateEntity("Target");
    const UUID id = entity.GetUUID();

    // Unknown entity -> failure.
    {
        PrefabOverrideSet set;
        set.AddOverride({UUID(424242ULL), "NameComponent", "Name", YAML::Node("Nope")});
        HK_CHECK(!set.Apply(scene));
    }

    // Unknown component -> failure.
    {
        PrefabOverrideSet set;
        set.AddOverride({id, "NotARealComponent", "Field", YAML::Node("x")});
        HK_CHECK(!set.Apply(scene));
    }

    // Unknown field on a real component -> failure.
    {
        PrefabOverrideSet set;
        set.AddOverride({id, "NameComponent", "Bogus", YAML::Node("x")});
        HK_CHECK(!set.Apply(scene));
    }

    // Valid override -> success and the value is applied.
    {
        PrefabOverrideSet set;
        set.AddOverride({id, "NameComponent", "Name", YAML::Node("Renamed")});
        HK_CHECK(static_cast<bool>(set.Apply(scene)));
        HK_CHECK_EQ(entity.GetName(), std::string("Renamed"));
    }
}

// End-to-end prefab override workflow: diff, revert, and apply (merge-back).
void RunPrefabApplyRevertTests() {
    HockeyTest::BeginSuite("PrefabApplyRevertTests");
    namespace fs = std::filesystem;

    const fs::path dir = Paths::TempFile("prefab_apply_revert_ws");
    FileSystem::Remove(dir);
    FileSystem::CreateDirectories(dir);
    const fs::path prefabPath = dir / "player.prefab.yaml";

    // Author a two-entity prefab (root + child) and save it.
    {
        Scene src("Src");
        Entity root = src.CreateEntity("Player");
        root.GetComponent<TransformComponent>().localPosition = {1.0f, 0.0f, 0.0f};
        Entity child = src.CreateEntity("Stick");
        src.SetParent(child, root, false);
        child.GetComponent<TransformComponent>().localPosition = {0.0f, 1.0f, 0.0f};
        HK_CHECK_MSG(static_cast<bool>(PrefabSerializer::Save(src, root, prefabPath)), "prefab saves");
    }

    Scene scene("Inst");
    Result<Entity> inst = PrefabSerializer::Instantiate(scene, prefabPath);
    HK_CHECK_MSG(static_cast<bool>(inst), "prefab instantiates");
    if (!inst) {
        FileSystem::Remove(dir);
        return;
    }
    Entity instance = inst.value;
    const UUID assetId = instance.GetComponent<PrefabComponent>().prefabAssetId;

    auto childOf = [&](Entity parent) -> Entity {
        const auto children = scene.GetChildren(parent);
        return children.empty() ? Entity{} : children.front();
    };
    Entity child = childOf(instance);
    HK_CHECK_MSG(child.IsValid(), "instance has the prefab child");

    // A fresh instance has no overrides.
    {
        PrefabOverrideSet set;
        Result<int> diff = PrefabSerializer::ComputeOverrides(scene, instance, set);
        HK_CHECK_MSG(static_cast<bool>(diff), "ComputeOverrides succeeds on a pristine instance");
        HK_CHECK_EQ(diff.value, 0);
    }

    // Edit the instance: rename the root and move the child.
    instance.SetName("PlayerEdited");
    child.GetComponent<TransformComponent>().localPosition = {9.0f, 9.0f, 9.0f};

    // The diff now records the diverging fields.
    {
        PrefabOverrideSet set;
        Result<int> diff = PrefabSerializer::ComputeOverrides(scene, instance, set);
        HK_CHECK_MSG(static_cast<bool>(diff), "ComputeOverrides succeeds after edits");
        HK_CHECK_MSG(diff.value >= 2, "name + child position diverge from prefab");
        bool sawName = false;
        for (const PrefabOverride& ov : set.Overrides()) {
            if (ov.componentName == "NameComponent" && ov.fieldName == "Name") {
                sawName = true;
            }
        }
        HK_CHECK_MSG(sawName, "diff captures the renamed root");
    }

    // Revert restores authored values while keeping identity and hierarchy.
    {
        HK_CHECK_MSG(static_cast<bool>(PrefabSerializer::RevertInstanceToPrefab(scene, instance)), "revert succeeds");
        HK_CHECK_EQ(instance.GetName(), std::string("Player"));
        const glm::vec3 childLocal = child.GetComponent<TransformComponent>().localPosition;
        HK_CHECK_NEAR(childLocal.y, 1.0f, 0.0001f);
        HK_CHECK_NEAR(childLocal.x, 0.0f, 0.0001f);
        HK_CHECK_EQ(scene.GetChildren(instance).size(), static_cast<std::size_t>(1));
        HK_CHECK_EQ(scene.GetParent(child).GetUUID(), instance.GetUUID());

        PrefabOverrideSet set;
        Result<int> diff = PrefabSerializer::ComputeOverrides(scene, instance, set);
        HK_CHECK_MSG(static_cast<bool>(diff) && diff.value == 0, "no overrides remain after revert");
    }

    // Apply merges instance edits back into the prefab, preserving its asset id.
    {
        instance.SetName("AppliedName");
        HK_CHECK_MSG(static_cast<bool>(PrefabSerializer::ApplyInstanceToPrefab(scene, instance)), "apply succeeds");

        Scene scene2("Inst2");
        Result<Entity> inst2 = PrefabSerializer::Instantiate(scene2, prefabPath);
        HK_CHECK_MSG(static_cast<bool>(inst2), "edited prefab re-instantiates");
        if (inst2) {
            HK_CHECK_EQ(inst2.value.GetName(), std::string("AppliedName"));
            HK_CHECK_MSG(inst2.value.GetComponent<PrefabComponent>().prefabAssetId == assetId,
                         "apply preserves the prefab asset id");
        }
    }

    FileSystem::Remove(dir);
}

// Duplicating a subtree emits one EntityCreated event per new entity.
void RunDuplicateEventTests() {
    HockeyTest::BeginSuite("DuplicateEventTests");

    Scene scene("DupEvents");
    Entity root = scene.CreateEntity("Root");
    Entity childA = scene.CreateEntity("ChildA");
    Entity childB = scene.CreateEntity("ChildB");
    scene.SetParent(childA, root, false);
    scene.SetParent(childB, root, false);

    scene.ClearPendingEvents();
    Entity copy = scene.DuplicateEntity(root);
    HK_CHECK(copy.IsValid());

    int created = 0;
    for (const SceneEvent& event : scene.GetPendingEvents()) {
        if (event.type == SceneEventType::EntityCreated) {
            ++created;
        }
    }
    // root + 2 children = 3 new entities.
    HK_CHECK_EQ(created, 3);
    HK_CHECK_EQ(scene.EntityCount(), static_cast<std::size_t>(6));
}

// SetParent to the current parent is a no-op (no transform/link changes).
void RunSetParentIdempotencyTests() {
    HockeyTest::BeginSuite("SetParentIdempotencyTests");

    Scene scene("Idempotent");
    Entity parent = scene.CreateEntity("Parent");
    parent.GetComponent<TransformComponent>().localPosition = {3.0f, 0.0f, 0.0f};
    Entity child = scene.CreateEntity("Child");
    scene.SetParent(child, parent, true);
    child.GetComponent<TransformComponent>().localPosition = {1.0f, 2.0f, 3.0f};

    const glm::vec3 localBefore = child.GetComponent<TransformComponent>().localPosition;
    const std::size_t childrenBefore = scene.GetChildren(parent).size();

    // Re-parent to the same parent: nothing should change.
    scene.SetParent(child, parent, true);

    const glm::vec3 localAfter = child.GetComponent<TransformComponent>().localPosition;
    HK_CHECK_NEAR(localAfter.x, localBefore.x, 0.0001f);
    HK_CHECK_NEAR(localAfter.y, localBefore.y, 0.0001f);
    HK_CHECK_NEAR(localAfter.z, localBefore.z, 0.0001f);
    HK_CHECK_EQ(scene.GetChildren(parent).size(), childrenBefore);
    HK_CHECK_EQ(scene.GetParent(child).GetUUID(), parent.GetUUID());
}
