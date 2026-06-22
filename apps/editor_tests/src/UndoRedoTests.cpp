#include "Test.hpp"

#include <filesystem>
#include <string>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorTransformOperations.hpp"
#include "Hockey/Editor/UndoRedo.hpp"

using namespace Hockey;

namespace {

// Builds a minimal editor context bound to a scene; no window/renderer needed
// for command logic.
struct CommandFixture {
    Scene scene{"UndoRedoTest"};
    EditorContext context;
    CommandFixture() {
        context.activeScene = &scene;
    }
};

glm::vec3 WorldPosition(Scene& scene, UUID id) {
    Entity entity = scene.FindEntityByUUID(id);
    return entity ? glm::vec3(scene.GetWorldTransform(entity)[3]) : glm::vec3(0.0f);
}

std::vector<UUID> EntityIds(const std::vector<Entity>& entities) {
    std::vector<UUID> ids;
    ids.reserve(entities.size());
    for (const Entity& entity : entities) {
        ids.push_back(entity.GetUUID());
    }
    return ids;
}

} // namespace

void RunUndoRedoTests() {
    HockeyTest::BeginSuite("UndoRedoTests");

    // Metadata drives Add/Remove component commands.
    ComponentRegistry::Get().RegisterPhase2Components();

    // --- stack basics: execute / undo / redo / clear ------------------------
    {
        CommandFixture fix;
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo(), "empty stack cannot undo");
        HK_CHECK_MSG(!fix.context.undoRedo.CanRedo(), "empty stack cannot redo");

        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("A"), fix.context);
        HK_CHECK_MSG(fix.context.undoRedo.CanUndo(), "after execute can undo");
        HK_CHECK_MSG(!fix.context.undoRedo.CanRedo(), "after execute cannot redo");
        HK_CHECK_EQ(fix.context.undoRedo.UndoName(), std::string("Create Entity"));
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo(), "after undo cannot undo");
        HK_CHECK_MSG(fix.context.undoRedo.CanRedo(), "after undo can redo");
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(0));

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));

        fix.context.undoRedo.Clear();
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo() && !fix.context.undoRedo.CanRedo(), "clear empties both stacks");
    }

    // --- create entity undo/redo preserves the UUID across redo -------------
    {
        CommandFixture fix;
        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("Created"), fix.context);
        const UUID id = fix.context.selection.Primary();
        HK_CHECK_MSG(id.IsValid() && fix.scene.ContainsUUID(id), "create selects and adds the entity");

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.ContainsUUID(id), "undo removes the created entity");

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.ContainsUUID(id), "redo restores the same UUID");
    }

    // --- new command clears the redo branch ---------------------------------
    {
        CommandFixture fix;
        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("A"), fix.context);
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(fix.context.undoRedo.CanRedo(), "redo available after undo");
        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("B"), fix.context);
        HK_CHECK_MSG(!fix.context.undoRedo.CanRedo(), "executing a new command clears redo");
    }

    // --- play-mode edits execute live but do not enter authoring history ----
    {
        CommandFixture fix;
        fix.context.playMode = true;
        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("LiveOnly"), fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo(), "play-mode edit is not recorded for authoring undo");
        HK_CHECK_MSG(!fix.context.sceneDirty, "play-mode edit does not mark the authored scene dirty");
    }

    // --- play mode blocks existing authoring undo/redo ----------------------
    {
        CommandFixture fix;
        fix.context.undoRedo.Execute(EditorCommands::CreateEntity("Authored"), fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));
        fix.context.playMode = true;
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(fix.context.undoRedo.CanUndo(), "authoring undo is preserved while play mode is active");
    }

    // --- rename entity undo/redo --------------------------------------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("Original");
        const UUID id = entity.GetUUID();
        fix.context.undoRedo.Execute(EditorCommands::RenameEntity(id, "Original", "Renamed"), fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetName(), std::string("Renamed"));
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetName(), std::string("Original"));
        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetName(), std::string("Renamed"));
    }

    // --- transform undo/redo ------------------------------------------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("Mover");
        const UUID id = entity.GetUUID();
        TransformData oldT;
        TransformData newT;
        newT.position = glm::vec3(1.0f, 2.0f, 3.0f);
        newT.scale = glm::vec3(2.0f);
        fix.context.undoRedo.Execute(EditorCommands::TransformEntity(id, oldT, newT), fix.context);
        {
            const auto& t = fix.scene.FindEntityByUUID(id).GetComponent<TransformComponent>();
            HK_CHECK_NEAR(t.localPosition.x, 1.0f, 1e-5);
            HK_CHECK_NEAR(t.localScale.y, 2.0f, 1e-5);
        }
        fix.context.undoRedo.Undo(fix.context);
        {
            const auto& t = fix.scene.FindEntityByUUID(id).GetComponent<TransformComponent>();
            HK_CHECK_NEAR(t.localPosition.x, 0.0f, 1e-5);
            HK_CHECK_NEAR(t.localScale.y, 1.0f, 1e-5);
        }
        fix.context.undoRedo.Redo(fix.context);
        {
            const auto& t = fix.scene.FindEntityByUUID(id).GetComponent<TransformComponent>();
            HK_CHECK_NEAR(t.localPosition.z, 3.0f, 1e-5);
        }
    }

    // --- grouped transform offsets selected roots together ------------------
    {
        CommandFixture fix;
        Entity a = fix.scene.CreateEntity("A");
        Entity b = fix.scene.CreateEntity("B");
        auto& aTransform = a.GetComponent<TransformComponent>();
        auto& bTransform = b.GetComponent<TransformComponent>();
        aTransform.localPosition = glm::vec3(1.0f, 0.0f, -2.0f);
        bTransform.localPosition = glm::vec3(-3.0f, 4.0f, 5.0f);

        const UUID aId = a.GetUUID();
        const UUID bId = b.GetUUID();
        fix.context.selection.Select(aId);
        fix.context.selection.Add(bId);

        std::vector<UUID> moved =
            EditorTransformOperations::TopLevelTransformableSelection(fix.scene, fix.context.selection);
        const glm::vec3 delta(5.0f, -1.0f, 2.0f);
        std::vector<EntityTransformSnapshot> snapshots =
            EditorTransformOperations::CaptureLocalSnapshots(fix.scene, moved);
        EditorTransformOperations::ApplyWorldTranslationDelta(fix.scene, moved, delta);
        EditorTransformOperations::CaptureCurrentAsAfter(fix.scene, snapshots);
        std::vector<EntityTransformSnapshot> changed = EditorTransformOperations::ChangedSnapshots(snapshots);

        HK_CHECK_NEAR(WorldPosition(fix.scene, aId).x, 6.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, aId).y, -1.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, aId).z, 0.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, bId).x, 2.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, bId).y, 3.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, bId).z, 7.0f, 1e-5);
        HK_CHECK_EQ(changed.size(), static_cast<std::size_t>(2));
    }

    // --- grouped transform moves selected parent branch once ----------------
    {
        CommandFixture fix;
        Entity parent = fix.scene.CreateEntity("Parent");
        Entity child = fix.scene.CreateEntity("Child");
        parent.GetComponent<TransformComponent>().localPosition =
            glm::vec3(10.0f, 0.0f, 0.0f);
        child.GetComponent<TransformComponent>().localPosition =
            glm::vec3(1.0f, 0.0f, 0.0f);
        fix.scene.SetParent(child, parent, /*keepWorldTransform=*/false);

        const UUID parentId = parent.GetUUID();
        const UUID childId = child.GetUUID();
        fix.context.selection.Select(parentId);
        fix.context.selection.Add(childId);

        std::vector<UUID> moved =
            EditorTransformOperations::TopLevelTransformableSelection(fix.scene, fix.context.selection);
        EditorTransformOperations::ApplyWorldTranslationDelta(
            fix.scene, moved, glm::vec3(3.0f, 0.0f, 0.0f));

        HK_CHECK_EQ(moved.size(), static_cast<std::size_t>(1));
        HK_CHECK_EQ(moved[0], parentId);
        HK_CHECK_NEAR(WorldPosition(fix.scene, parentId).x, 13.0f, 1e-5);
        HK_CHECK_NEAR(WorldPosition(fix.scene, childId).x, 14.0f, 1e-5);
        HK_CHECK_NEAR(child.GetComponent<TransformComponent>().localPosition.x,
                      1.0f, 1e-5);
    }

    // --- grouped transform undo/redo restores every moved entity ------------
    {
        CommandFixture fix;
        Entity a = fix.scene.CreateEntity("A");
        Entity b = fix.scene.CreateEntity("B");
        const UUID aId = a.GetUUID();
        const UUID bId = b.GetUUID();

        TransformData aBefore;
        TransformData bBefore;
        TransformData aAfter;
        TransformData bAfter;
        aAfter.position = glm::vec3(2.0f, 0.0f, 0.0f);
        bAfter.position = glm::vec3(-1.0f, 3.0f, 0.0f);

        fix.context.undoRedo.Execute(
            EditorCommands::TransformEntities({{aId, aBefore, aAfter}, {bId, bBefore, bAfter}}), fix.context);

        HK_CHECK_NEAR(a.GetComponent<TransformComponent>().localPosition.x, 2.0f, 1e-5);
        HK_CHECK_NEAR(b.GetComponent<TransformComponent>().localPosition.y, 3.0f, 1e-5);

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_NEAR(a.GetComponent<TransformComponent>().localPosition.x, 0.0f, 1e-5);
        HK_CHECK_NEAR(b.GetComponent<TransformComponent>().localPosition.y, 0.0f, 1e-5);

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_NEAR(a.GetComponent<TransformComponent>().localPosition.x, 2.0f, 1e-5);
        HK_CHECK_NEAR(b.GetComponent<TransformComponent>().localPosition.y, 3.0f, 1e-5);
    }

    // --- add component undo/redo --------------------------------------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("WithMesh");
        const UUID id = entity.GetUUID();
        fix.context.undoRedo.Execute(EditorCommands::AddComponent(id, "MeshRendererComponent"), fix.context);
        HK_CHECK_MSG(fix.scene.FindEntityByUUID(id).HasComponent<MeshRendererComponent>(), "add component present");
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.FindEntityByUUID(id).HasComponent<MeshRendererComponent>(), "undo removes component");
        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.FindEntityByUUID(id).HasComponent<MeshRendererComponent>(), "redo re-adds component");
    }

    // --- remove component undo/redo preserves component data ----------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("RemoveMe");
        const UUID id = entity.GetUUID();
        entity.AddComponent<MeshRendererComponent>().meshName = "puck.mesh";

        fix.context.undoRedo.Execute(EditorCommands::RemoveComponent(fix.scene, id, "MeshRendererComponent"),
                                     fix.context);
        HK_CHECK_MSG(!fix.scene.FindEntityByUUID(id).HasComponent<MeshRendererComponent>(),
                     "execute removes component");

        fix.context.undoRedo.Undo(fix.context);
        Entity restored = fix.scene.FindEntityByUUID(id);
        HK_CHECK_MSG(restored.HasComponent<MeshRendererComponent>(), "undo restores component");
        if (restored.HasComponent<MeshRendererComponent>()) {
            HK_CHECK_EQ(restored.GetComponent<MeshRendererComponent>().meshName, std::string("puck.mesh"));
        }

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(!fix.scene.FindEntityByUUID(id).HasComponent<MeshRendererComponent>(), "redo removes again");
    }

    // --- edit component field undo/redo (snapshot based) --------------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("FieldEdit");
        const UUID id = entity.GetUUID();
        entity.AddComponent<MeshRendererComponent>().meshName = "before.mesh";

        const std::string before = EntitySnapshot::CaptureEntity(fix.scene, id);
        fix.scene.FindEntityByUUID(id).GetComponent<MeshRendererComponent>().meshName = "after.mesh";
        const std::string after = EntitySnapshot::CaptureEntity(fix.scene, id);

        fix.context.undoRedo.Execute(EditorCommands::EditComponentField(id, "Mesh Renderer", before, after),
                                     fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetComponent<MeshRendererComponent>().meshName,
                    std::string("after.mesh"));
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetComponent<MeshRendererComponent>().meshName,
                    std::string("before.mesh"));
        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(id).GetComponent<MeshRendererComponent>().meshName,
                    std::string("after.mesh"));
    }

    // --- delete entity undo/redo restores subtree, UUIDs and hierarchy ------
    {
        CommandFixture fix;
        Entity parent = fix.scene.CreateEntity("Parent");
        Entity child = fix.scene.CreateEntity("Child");
        fix.scene.SetParent(child, parent);
        const UUID parentId = parent.GetUUID();
        const UUID childId = child.GetUUID();

        fix.context.undoRedo.Execute(EditorCommands::DeleteEntity(fix.scene, parentId), fix.context);
        HK_CHECK_MSG(!fix.scene.ContainsUUID(parentId) && !fix.scene.ContainsUUID(childId),
                     "delete removes the whole subtree");

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(fix.scene.ContainsUUID(parentId) && fix.scene.ContainsUUID(childId),
                     "undo restores subtree with original UUIDs");
        Entity restoredChild = fix.scene.FindEntityByUUID(childId);
        HK_CHECK_MSG(restoredChild && fix.scene.GetParent(restoredChild).GetUUID() == parentId,
                     "undo restores the parent/child link");

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(!fix.scene.ContainsUUID(parentId) && !fix.scene.ContainsUUID(childId), "redo deletes again");
    }

    // --- duplicate undo/redo ------------------------------------------------
    {
        CommandFixture fix;
        Entity source = fix.scene.CreateEntity("Source");
        const UUID sourceId = source.GetUUID();
        fix.context.undoRedo.Execute(EditorCommands::DuplicateEntity(sourceId), fix.context);
        const UUID copyId = fix.context.selection.Primary();
        HK_CHECK_MSG(copyId.IsValid() && copyId != sourceId, "duplicate creates a new UUID");
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(2));

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(!fix.scene.ContainsUUID(copyId), "undo removes the duplicate");

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.ContainsUUID(copyId), "redo restores the duplicate with the same UUID");
    }

    // --- set parent undo/redo -----------------------------------------------
    {
        CommandFixture fix;
        Entity a = fix.scene.CreateEntity("A");
        Entity b = fix.scene.CreateEntity("B");
        const UUID aId = a.GetUUID();
        const UUID bId = b.GetUUID();
        fix.context.undoRedo.Execute(EditorCommands::SetParent(fix.scene, aId, bId), fix.context);
        HK_CHECK_MSG(fix.scene.GetParent(fix.scene.FindEntityByUUID(aId)).GetUUID() == bId, "set parent links a->b");
        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.GetParent(fix.scene.FindEntityByUUID(aId)), "undo returns a to root");
        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.GetParent(fix.scene.FindEntityByUUID(aId)).GetUUID() == bId, "redo re-links a->b");
    }

    // --- move entity undo/redo restores sibling order -----------------------
    {
        CommandFixture fix;
        Entity parent = fix.scene.CreateEntity("Parent");
        Entity a = fix.scene.CreateEntity("A");
        Entity b = fix.scene.CreateEntity("B");
        Entity c = fix.scene.CreateEntity("C");
        fix.scene.SetParent(a, parent, false);
        fix.scene.SetParent(b, parent, false);
        fix.scene.SetParent(c, parent, false);

        const UUID aId = a.GetUUID();
        const UUID bId = b.GetUUID();
        const UUID cId = c.GetUUID();

        fix.context.undoRedo.Execute(EditorCommands::MoveEntity(fix.scene, cId, parent.GetUUID(), 0), fix.context);
        {
            const std::vector<UUID> children = EntityIds(fix.scene.GetChildren(parent));
            HK_CHECK(children[0] == cId);
            HK_CHECK(children[1] == aId);
            HK_CHECK(children[2] == bId);
        }

        fix.context.undoRedo.Undo(fix.context);
        {
            const std::vector<UUID> children = EntityIds(fix.scene.GetChildren(parent));
            HK_CHECK(children[0] == aId);
            HK_CHECK(children[1] == bId);
            HK_CHECK(children[2] == cId);
        }

        fix.context.undoRedo.Redo(fix.context);
        {
            const std::vector<UUID> children = EntityIds(fix.scene.GetChildren(parent));
            HK_CHECK(children[0] == cId);
            HK_CHECK(children[1] == aId);
            HK_CHECK(children[2] == bId);
        }
    }

    // --- move entity undo/redo restores root order --------------------------
    {
        CommandFixture fix;
        Entity a = fix.scene.CreateEntity("A");
        Entity b = fix.scene.CreateEntity("B");
        Entity c = fix.scene.CreateEntity("C");
        const UUID aId = a.GetUUID();
        const UUID bId = b.GetUUID();
        const UUID cId = c.GetUUID();

        fix.context.undoRedo.Execute(EditorCommands::MoveEntity(fix.scene, cId, UUID(0), 0), fix.context);
        {
            const std::vector<UUID> roots = EntityIds(fix.scene.GetRootEntities());
            HK_CHECK(roots[0] == cId);
            HK_CHECK(roots[1] == aId);
            HK_CHECK(roots[2] == bId);
        }

        fix.context.undoRedo.Undo(fix.context);
        {
            const std::vector<UUID> roots = EntityIds(fix.scene.GetRootEntities());
            HK_CHECK(roots[0] == aId);
            HK_CHECK(roots[1] == bId);
            HK_CHECK(roots[2] == cId);
        }

        fix.context.undoRedo.Redo(fix.context);
        {
            const std::vector<UUID> roots = EntityIds(fix.scene.GetRootEntities());
            HK_CHECK(roots[0] == cId);
            HK_CHECK(roots[1] == aId);
            HK_CHECK(roots[2] == bId);
        }
    }

    // --- clipboard copy + paste creates a fresh-UUID copy -------------------
    {
        CommandFixture fix;
        Entity original = fix.scene.CreateEntity("Clip");
        const UUID id = original.GetUUID();
        fix.context.clipboard.CopyEntities(fix.scene, {id});
        HK_CHECK_MSG(fix.context.clipboard.HasEntities(), "clipboard holds the copied entity");

        const std::string snapshot = fix.context.clipboard.EntitySnapshots().front();
        fix.context.undoRedo.Execute(EditorCommands::PasteEntity(snapshot, UUID(0)), fix.context);
        const UUID pasted = fix.context.selection.Primary();
        HK_CHECK_MSG(pasted.IsValid() && pasted != id, "paste yields a new UUID");
        HK_CHECK_EQ(fix.scene.FindEntityByUUID(pasted).GetName(), std::string("Clip"));

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.ContainsUUID(pasted), "undo removes the pasted entity");
        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.ContainsUUID(pasted), "redo restores the pasted entity");
    }

    // --- clipboard copy component + paste onto another entity ----------------
    {
        CommandFixture fix;
        Entity source = fix.scene.CreateEntity("CompSource");
        Entity target = fix.scene.CreateEntity("CompTarget");
        source.AddComponent<MeshRendererComponent>().meshName = "shared.mesh";
        const UUID targetId = target.GetUUID();

        fix.context.clipboard.CopyComponent(fix.scene, source.GetUUID(), "MeshRendererComponent");
        HK_CHECK_MSG(fix.context.clipboard.HasComponent(), "clipboard holds copied component");

        fix.context.undoRedo.Execute(EditorCommands::PasteComponent(fix.scene, targetId, "MeshRendererComponent",
                                                                    fix.context.clipboard.ComponentYaml()),
                                     fix.context);
        Entity pastedTarget = fix.scene.FindEntityByUUID(targetId);
        HK_CHECK_MSG(pastedTarget.HasComponent<MeshRendererComponent>(), "paste adds the component to target");
        if (pastedTarget.HasComponent<MeshRendererComponent>()) {
            HK_CHECK_EQ(pastedTarget.GetComponent<MeshRendererComponent>().meshName, std::string("shared.mesh"));
        }

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.FindEntityByUUID(targetId).HasComponent<MeshRendererComponent>(),
                     "undo removes the pasted component that was not present before");
    }

    // --- create prefab links the source entity, undo unlinks it -------------
    {
        CommandFixture fix;
        Entity entity = fix.scene.CreateEntity("PrefabSource");
        entity.AddComponent<MeshRendererComponent>().meshName = "puck.mesh";
        const UUID id = entity.GetUUID();
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "hk_create_prefab_test.prefab.yaml";
        std::error_code ec;
        std::filesystem::remove(path, ec);

        fix.context.undoRedo.Execute(EditorCommands::CreatePrefab(id, path), fix.context);
        HK_CHECK_MSG(std::filesystem::exists(path), "create prefab writes the asset file");
        Entity stamped = fix.scene.FindEntityByUUID(id);
        HK_CHECK_MSG(stamped.HasComponent<PrefabComponent>(), "create prefab stamps the source entity");
        if (stamped.HasComponent<PrefabComponent>()) {
            HK_CHECK_MSG(stamped.GetComponent<PrefabComponent>().sourcePath == path,
                         "prefab component records the asset path");
        }

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_MSG(!fix.scene.FindEntityByUUID(id).HasComponent<PrefabComponent>(),
                     "undo unlinks the source entity from the prefab");

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_MSG(fix.scene.FindEntityByUUID(id).HasComponent<PrefabComponent>(),
                     "redo re-links the source entity to the prefab");

        std::filesystem::remove(path, ec);
    }

    // --- revert prefab overrides restores authored values; undo brings them back -
    {
        CommandFixture fix;
        Entity source = fix.scene.CreateEntity("RevertSource");
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "hk_revert_prefab_test.prefab.yaml";
        std::error_code ec;
        std::filesystem::remove(path, ec);

        // Author the prefab from the source entity, then instantiate a fresh copy.
        HK_CHECK_MSG(static_cast<bool>(PrefabSerializer::Save(fix.scene, source, path)), "revert test prefab saves");
        Result<Entity> inst = PrefabSerializer::Instantiate(fix.scene, path);
        HK_CHECK_MSG(static_cast<bool>(inst), "revert test prefab instantiates");
        if (inst) {
            Entity instance = inst.value;
            const UUID instanceId = instance.GetUUID();
            instance.SetName("RevertEdited");

            fix.context.undoRedo.Execute(EditorCommands::RevertPrefabOverrides(fix.scene, instanceId), fix.context);
            HK_CHECK_MSG(fix.scene.FindEntityByUUID(instanceId).GetName() == "RevertSource",
                         "revert restores the authored name");

            fix.context.undoRedo.Undo(fix.context);
            HK_CHECK_MSG(fix.scene.FindEntityByUUID(instanceId).GetName() == "RevertEdited",
                         "undo restores the edited name");

            fix.context.undoRedo.Redo(fix.context);
            HK_CHECK_MSG(fix.scene.FindEntityByUUID(instanceId).GetName() == "RevertSource",
                         "redo re-applies the revert");
        }
        std::filesystem::remove(path, ec);
    }
}
